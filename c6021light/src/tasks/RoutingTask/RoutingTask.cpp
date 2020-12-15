#include "tasks/RoutingTask/RoutingTask.h"

#include <atomic>
#include <cstdint>
#include <cstdio>

#include <LocoNet.h>
#include "RR32Can/RR32Can.h"
#include "hal/stm32I2C.h"
#include "hal/stm32can.h"

#include "OsQueue.h"

namespace tasks {
namespace RoutingTask {

RoutingTask* RoutingTask::lnCallbackInstance;

/**
 * \brief Two addresses are on the same decoder, if they match apart from
 * the lowest two bits.
 */
bool sameDecoder(RR32Can::MachineTurnoutAddress left, RR32Can::MachineTurnoutAddress right) {
  constexpr const uint8_t mask = 0xFC;
  return (left.value() & mask) == (right.value() & mask);
}

MarklinI2C::Messages::AccessoryMsg RoutingTask::getI2CMessage(hal::I2CBuf& buffer) {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.destination_ = dataModel_->myAddr;
  msg.source_ = buffer.msgBytes[0];
  msg.data_ = buffer.msgBytes[1];
  return msg;
}

MarklinI2C::Messages::AccessoryMsg RoutingTask::prepareI2cMessage() {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.source_ = dataModel_->myAddr;
  return msg;
}

void RoutingTask::SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg) {
  printf("I2C TX: ");
  msg.print();

  hal::I2CBuf buf;
  buf.msgBytes[0] = msg.destination_ >> 1;
  buf.msgBytes[1] = msg.data_;

  hal::i2cTxQueue.Send(buf, 0);  // TODO: Check the result.
  hal::startTx();
}

void RoutingTask::ForwardToI2C(const RR32Can::Identifier rr32id, const RR32Can::Data& rr32data) {
  switch (rr32id.getCommand()) {
    case RR32Can::Command::ACCESSORY_SWITCH: {
      const RR32Can::TurnoutPacket turnoutPacket(const_cast<RR32Can::Data&>(rr32data));
      if (rr32id.isResponse()) {
        // Responses are forwarded to I2C
        printf(" Got an Accessory packet!\n");

        if (turnoutPacket.getRailProtocol() != RR32Can::RailProtocol::MM1) {
          // Not an MM2 packet
          return;
        }

        RR32Can::MachineTurnoutAddress turnoutAddr = turnoutPacket.getLocid().getNumericAddress();
        if (turnoutAddr.value() > 0xFF) {
          // Addr too large for the i2c bus.
          return;
        }

        // Convert to i2c confirmation packet
        MarklinI2C::Messages::AccessoryMsg i2cMsg = prepareI2cMessage();

        i2cMsg.setTurnoutAddr(turnoutAddr);
        i2cMsg.setPower(turnoutPacket.getPower());
        // Direction is not transmitted on Response.
        i2cMsg.setDirection(turnoutPacket.getDirection());
        i2cMsg.makePowerConsistent();

        SendI2CMessage(i2cMsg);
      }
      break;
    }

    default:
      // Other messages not forwarded.
      break;
  }
}

void printLnPacket(lnMsg* LnPacket) {
  printf("LN RX: ");
  for (int i = 0; i < getLnMsgSize(LnPacket); ++i) {
    printf(" %x", LnPacket->data[i]);
  }
  printf("\n");
}

void RoutingTask::MakeRR32CanMsg(const MarklinI2C::Messages::AccessoryMsg& request,
                                 RR32Can::Identifier& rr32id, RR32Can::Data& rr32data) {
  rr32id.setCommand(RR32Can::Command::ACCESSORY_SWITCH);
  rr32id.setResponse(false);

  RR32Can::TurnoutPacket turnoutPacket(rr32data);
  turnoutPacket.initData();

  // If this is a power ON packet: Send directly to CAN
  if (request.getPower()) {
    lastPowerOnDirection = request.getDirection();
    lastPowerOnTurnoutAddr = RR32Can::MachineTurnoutAddress(request.getTurnoutAddr());

    RR32Can::MachineTurnoutAddress protocolAddr = lastPowerOnTurnoutAddr;
    protocolAddr.setProtocol(dataModel_->accessoryRailProtocol);

    turnoutPacket.setLocid(protocolAddr);
    turnoutPacket.setPower(request.getPower());
    turnoutPacket.setDirection(request.getDirection());

  } else {
    // On I2C, for a Power OFF message, the two lowest bits (decoder output channel) are always
    // 0, regardless of the actual turnout address to be switched off.
    //
    // Note that we store the last direction where power was applied and only turn off that.
    // The CAN side interprets a "Power Off" as "Flip the switch" anyways.
    RR32Can::MachineTurnoutAddress i2cAddr = request.getTurnoutAddr();
    if (sameDecoder(i2cAddr, lastPowerOnTurnoutAddr.value())) {
      RR32Can::MachineTurnoutAddress protocolAddr = lastPowerOnTurnoutAddr;
      protocolAddr.setProtocol(dataModel_->accessoryRailProtocol);

      turnoutPacket.setLocid(protocolAddr);
      turnoutPacket.setPower(request.getPower());
      turnoutPacket.setDirection(lastPowerOnDirection);

    } else {
      printf("PowerOff for wrong decoder.\n");
    }
  }
}

void RoutingTask::ForwardToLoconet(const RR32Can::Identifier rr32id,
                                   const RR32Can::Data& rr32data) {
  switch (rr32id.getCommand()) {
    case RR32Can::Command::ACCESSORY_SWITCH: {
      const RR32Can::TurnoutPacket turnoutPacket(const_cast<RR32Can::Data&>(rr32data));
      if (!rr32id.isResponse()) {
        // Send to LocoNet
        LocoNet.requestSwitch(
            RR32Can::HumanTurnoutAddress(turnoutPacket.getLocid().getNumericAddress()).value(),
            turnoutPacket.getPower(),
            RR32Can::TurnoutDirectionToIntegral<uint8_t>(turnoutPacket.getDirection()));
      }
      break;
    }

    case RR32Can::Command::SYSTEM_COMMAND: {
      const RR32Can::SystemMessage systemMessage(const_cast<RR32Can::Data&>(rr32data));
      switch (systemMessage.getSubcommand()) {
        case RR32Can::SystemSubcommand::SYSTEM_STOP:
          if (!rr32id.isResponse()) {
            LocoNet.reportPower(false);
          }
          break;
        case RR32Can::SystemSubcommand::SYSTEM_GO:
          if (!rr32id.isResponse()) {
            LocoNet.reportPower(true);
          }
          break;
        default:
          // Other messages not forwarded.
          break;
      }
    }
    default:
      // Other messages not forwarded.
      break;
  }
}

/**
 * \brief When a message was received, create and send a response message.
 */
void RoutingTask::TaskMain() {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait until someone sends us a notification.

    // Process CAN
    constexpr const TickType_t ticksToWait = 0;
    for (hal::CanQueueType::ReceiveResult receiveResult = hal::canrxq.Receive(ticksToWait);
         receiveResult.errorCode == pdTRUE; receiveResult = hal::canrxq.Receive(ticksToWait)) {
      RR32Can::Identifier rr32id = RR32Can::Identifier::GetIdentifier(receiveResult.element.id);
      RR32Can::RR32Can.HandlePacket(rr32id, receiveResult.element.data);

      ForwardToI2C(rr32id, receiveResult.element.data);
      ForwardToLoconet(rr32id, receiveResult.element.data);
    }

    // Process I2C
    {
      hal::I2CQueueType::ReceiveResult receiveResult = hal::i2cRxQueue.Receive(0);
      if (receiveResult.errorCode == pdPASS) {
        MarklinI2C::Messages::AccessoryMsg request = getI2CMessage(receiveResult.element);
        printf("I2C RX: ");
        request.print();

        RR32Can::Identifier rr32id;
        RR32Can::Data rr32data;

        // Convert to generic CAN representation
        MakeRR32CanMsg(request, rr32id, rr32data);
        // Forward to CAN
        RR32Can::RR32Can.SendPacket(rr32id, rr32data);
        // Forward to LocoNet
        ForwardToLoconet(rr32id, rr32data);
      }
    }

    // Process LocoNet
    {
      lnMsg* LnPacket = LocoNet.receive();
      if (LnPacket) {
        printLnPacket(LnPacket);
        LocoNet.processSwitchSensorMessage(LnPacket);
      }
    }
  }
}

/**
 * \brief Notification Function used by LocoNet to indicate a Stop/Go message.
 */
extern "C" void notifyPower(uint8_t State) {
  tasks::RoutingTask::RoutingTask::lnCallbackInstance->LocoNetNotifyPower(State);
}

void RoutingTask::LocoNetNotifyPower(uint8_t State) {
  if (State != 0) {
    RR32Can::RR32Can.SendSystemGo();
  } else {
    RR32Can::RR32Can.SendSystemStop();
  }
}

// Address: Switch Address.
// Output: Value 0 for Coil Off, anything else for Coil On
// Direction: Value 0 for Closed/GREEN, anything else for Thrown/RED
extern "C" void notifySwitchRequest(uint16_t Address, uint8_t Output, uint8_t Direction) {
  tasks::RoutingTask::RoutingTask::lnCallbackInstance->LocoNetNotifySwitchRequest(Address, Output,
                                                                                  Direction);
}

void RoutingTask::LocoNetNotifySwitchRequest(uint16_t LnAddress, uint8_t Output,
                                             uint8_t LnDirection) {
  // Send to CAN
  RR32Can::MachineTurnoutAddress address = RR32Can::HumanTurnoutAddress(LnAddress);
  RR32Can::TurnoutDirection dir;
  if (LnDirection != 0) {
    dir = RR32Can::TurnoutDirection::GREEN;
  } else {
    dir = RR32Can::TurnoutDirection::RED;
  }
  if (Output != 0) {
    Output = 1;
  }
  RR32Can::RR32Can.SendAccessoryPacket(address, dataModel_->accessoryRailProtocol, dir, Output);

  // Send to I2C
  MarklinI2C::Messages::AccessoryMsg i2cMsg = prepareI2cMessage();

  i2cMsg.setTurnoutAddr(address.value());
  i2cMsg.setPower(Output);
  i2cMsg.setDirection(dir);
  i2cMsg.makePowerConsistent();

  SendI2CMessage(i2cMsg);
}

}  // namespace RoutingTask
}  // namespace tasks
