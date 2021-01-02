#include "tasks/RoutingTask/RoutingTask.h"

#include <atomic>
#include <cstdint>
#include <cstdio>

#include <LocoNet.h>
#include "RR32Can/RR32Can.h"
#include "RR32Can/messages/S88Event.h"
#include "hal/stm32I2C.h"
#include "hal/stm32can.h"
#include "tasks/RoutingTask/LocoNetPrinter.h"

#include "OsQueue.h"

namespace tasks {
namespace RoutingTask {

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

bool RoutingTask::MakeRR32CanMsg(const MarklinI2C::Messages::AccessoryMsg& request,
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
  return true;
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
            RR32Can::TurnoutDirectionToIntegral(turnoutPacket.getDirection()));
      }
      break;
    }

    case RR32Can::Command::SYSTEM_COMMAND: {
      const RR32Can::SystemMessage systemMessage(const_cast<RR32Can::Data&>(rr32data));
      switch (systemMessage.getSubcommand()) {
        case RR32Can::SystemSubcommand::SYSTEM_STOP:
          LocoNet.reportPower(false);
          break;
        case RR32Can::SystemSubcommand::SYSTEM_GO:
          LocoNet.reportPower(true);
          break;
        default:
          // Other messages not forwarded.
          break;
      }
      break;
    }
    case RR32Can::Command::S88_EVENT: {
      const RR32Can::S88Event s88Event(const_cast<RR32Can::Data&>(rr32data));
      if (s88Event.getSubtype() == RR32Can::S88Event::Subtype::RESPONSE) {
        uint8_t state = 0;
        switch (s88Event.getNewState()) {
          case RR32Can::S88Event::State::OPEN:
            state = 0;
            break;
          case RR32Can::S88Event::State::CLOSED:
            state = 1;
            break;
        }
        LocoNet.reportSensor(RR32Can::HumanTurnoutAddress(s88Event.getContactId()).value(), state);
      }
      break;
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
    waitForNotify();

    // Process CAN
    constexpr const TickType_t ticksToWait = 0;
    for (hal::CanQueueType::ReceiveResult receiveResult = hal::canrxq.Receive(ticksToWait);
         receiveResult.errorCode == pdTRUE; receiveResult = hal::canrxq.Receive(ticksToWait)) {
      ForwardToI2C(receiveResult.element.id, receiveResult.element.data);
      ForwardToLoconet(receiveResult.element.id, receiveResult.element.data);
      // Forward to self
      RR32Can::RR32Can.HandlePacket(receiveResult.element.id, receiveResult.element.data);
    }

    // Process I2C
    for (hal::I2CQueueType::ReceiveResult receiveResult = hal::i2cRxQueue.Receive(0);
         receiveResult.errorCode == pdPASS; receiveResult = hal::i2cRxQueue.Receive(0)) {
      MarklinI2C::Messages::AccessoryMsg request = getI2CMessage(receiveResult.element);
      printf("I2C RX: ");
      request.print();

      RR32Can::Identifier rr32id;
      RR32Can::Data rr32data;

      // Convert to generic CAN representation
      if (MakeRR32CanMsg(request, rr32id, rr32data)) {
        RR32Can::RR32Can.SendPacket(rr32id, rr32data);
        ForwardToLoconet(rr32id, rr32data);
        // Forward to self
        RR32Can::RR32Can.HandlePacket(rr32id, rr32data);
      }
    }

    // Process LocoNet
    for (lnMsg* LnPacket = LocoNet.receive(); LnPacket; LnPacket = LocoNet.receive()) {
      printLnPacket(*LnPacket);

      RR32Can::Identifier rr32id;
      RR32Can::Data rr32data;

      // Convert to generic CAN representation
      if (MakeRR32CanMsg(*LnPacket, rr32id, rr32data)) {
        ForwardToI2C(rr32id, rr32data);
        // Forward to CAN
        RR32Can::RR32Can.SendPacket(rr32id, rr32data);

        // Forward to self
        RR32Can::RR32Can.HandlePacket(rr32id, rr32data);
      }
    }
  }
}

bool RoutingTask::MakeRR32CanMsg(const lnMsg& LnPacket, RR32Can::Identifier& rr32id,
                                 RR32Can::Data& rr32data) {
  // Decode the opcode
  switch (LnPacket.data[0]) {
    case OPC_SW_REQ: {
      rr32id.setCommand(RR32Can::Command::ACCESSORY_SWITCH);
      rr32id.setResponse(false);
      RR32Can::TurnoutPacket turnoutPacket(rr32data);
      turnoutPacket.initData();

      // Extract the switch address
      RR32Can::MachineTurnoutAddress lnAddr = ((LnPacket.srq.sw2 & 0x0F) << 7) | LnPacket.srq.sw1;
      lnAddr.setProtocol(dataModel_->accessoryRailProtocol);
      turnoutPacket.setLocid(lnAddr);

      RR32Can::TurnoutDirection direction =
          ((LnPacket.srq.sw2 & OPC_SW_REQ_DIR) == 0 ? RR32Can::TurnoutDirection::RED
                                                    : RR32Can::TurnoutDirection::GREEN);
      turnoutPacket.setDirection(direction);
      uint8_t power = LnPacket.srq.sw2 & OPC_SW_REQ_OUT;
      turnoutPacket.setPower(power);

      return true;
      break;
    }
    case OPC_GPON:
    case OPC_GPOFF: {
      rr32id.setCommand(RR32Can::Command::SYSTEM_COMMAND);
      rr32id.setResponse(false);
      RR32Can::SystemMessage systemMessage(rr32data);
      systemMessage.initData();

      if (LnPacket.data[0] == OPC_GPON) {
        systemMessage.setSubcommand(RR32Can::SystemSubcommand::SYSTEM_GO);
      } else {
        systemMessage.setSubcommand(RR32Can::SystemSubcommand::SYSTEM_STOP);
      }

      return true;
      break;
    }
    case OPC_INPUT_REP: {
      rr32id.setCommand(RR32Can::Command::S88_EVENT);
      rr32id.setResponse(true);
      RR32Can::S88Event message(rr32data);
      message.initData();
      message.setSubtype(RR32Can::S88Event::Subtype::RESPONSE);
      message.setDeviceId(0);

      {
        uint16_t addr = LnPacket.ir.in1 << 1;
        addr |= (LnPacket.ir.in2 & 0x0F) << 8;
        addr |= (LnPacket.ir.in2 & 0x20) >> 5;
        RR32Can::MachineTurnoutAddress lnAddr(addr);

        message.setContactId(lnAddr);
      }
      {
        RR32Can::S88Event::State newState;
        RR32Can::S88Event::State oldState;
        if (((LnPacket.ir.in2 & 0x10) >> 4) == 0) {
          newState = RR32Can::S88Event::State::OPEN;
          oldState = RR32Can::S88Event::State::CLOSED;
        } else {
          newState = RR32Can::S88Event::State::CLOSED;
          oldState = RR32Can::S88Event::State::OPEN;
        }

        message.setStates(oldState, newState);
      }
      message.setTime(0);
      return true;
      break;
    }
    default:
      // Other packet types not handled.
      return false;
      break;
  }
}

}  // namespace RoutingTask
}  // namespace tasks
