#include "tasks/RoutingTask/RoutingTask.h"

#include <atomic>
#include <cstdint>
#include <cstdio>

#include <LocoNet.h>
#include "RR32Can/RR32Can.h"
#include "hal/stm32I2C.h"

#include "OsQueue.h"

namespace tasks {
namespace RoutingTask {

RoutingTask* RoutingTask::lnCallbackInstance;

/**
 * \brief Two addresses are on the same decoder, if they match apart from
 * the lowest two bits.
 */
bool sameDecoder(uint8_t left, uint8_t right) {
  constexpr const uint8_t mask = 0xFC;
  return (left & mask) == (right & mask);
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
  hal::I2CBuf buf;
  buf.msgBytes[0] = msg.destination_;
  buf.msgBytes[1] = msg.data_;

  hal::i2cTxQueue.Send(buf, 0);  // TODO: Check the result.
  hal::startTx();
}

void RoutingTask::OnAccessoryPacket(RR32Can::TurnoutPacket& packet, bool response) {
  if (!response) {
    // Requests are forwarded to LocoNet
    LocoNet.requestSwitch(
        RR32Can::HumanTurnoutAddress(packet.locid).value() & 0x03FF, packet.power,
        static_cast<std::underlying_type<RR32Can::TurnoutDirection>::type>(packet.position));
  } else {
    // Responses are forwarded to I2C
    printf(" Got an Accessory packet!");

    if (packet.getRailProtocol() != RR32Can::RailProtocol::MM1) {
      // Not an MM2 packet
      return;
    }

    uint16_t turnoutAddr = packet.locid.value() & 0x03FF;
    if (turnoutAddr > 0xFF) {
      // Addr too large for the i2c bus.
      return;
    }

    // Convert to i2c confirmation packet
    MarklinI2C::Messages::AccessoryMsg i2cMsg = prepareI2cMessage();

    i2cMsg.setTurnoutAddr(turnoutAddr);
    i2cMsg.setPower(packet.power);
    // Direction is not transmitted on Response.
    i2cMsg.setDirection(
        static_cast<std::underlying_type<RR32Can::TurnoutDirection>::type>(packet.position));
    i2cMsg.makePowerConsistent();

    SendI2CMessage(i2cMsg);
  }
}

void printLnPacket(lnMsg* LnPacket) {
  printf("LN RX: ");
  for (int i = 0; i < getLnMsgSize(LnPacket); ++i) {
    printf(" %x", LnPacket->data[i]);
  }
  printf("\n");
}

/**
 * \brief When a message was received, create and send a response message.
 */
void RoutingTask::TaskMain() {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait until someone sends us a notification.
    halImpl_->loopCan();

    // Process I2C
    hal::I2CQueueType::ReceiveResult receiveResult = hal::i2cRxQueue.Receive(0);
    if (receiveResult.errorCode == pdPASS) {
      MarklinI2C::Messages::AccessoryMsg request = getI2CMessage(receiveResult.element);
      request.print();
      // If this is a power ON packet: Send directly to CAN
      if (request.getPower()) {
        lastPowerOnDirection = request.getDirection();
        lastPowerOnTurnoutAddr = RR32Can::MachineTurnoutAddress(request.getTurnoutAddr());
        RR32Can::RR32Can.SendAccessoryPacket(
            lastPowerOnTurnoutAddr, dataModel_->accessoryRailProtocol,
            static_cast<RR32Can::TurnoutDirection>(request.getDirection()), request.getPower());
      } else {
        // On I2C, for a Power OFF message, the two lowest bits (decoder output channel) are always
        // 0, regardless of the actual turnout address to be switched off.
        //
        // Note that we store the last direction where power was applied and only turn off that.
        // The CAN side interprets a "Power Off" as "Flip the switch" anyways.
        uint8_t i2cAddr = request.getTurnoutAddr();
        if (sameDecoder(i2cAddr, lastPowerOnTurnoutAddr.value())) {
          RR32Can::RR32Can.SendAccessoryPacket(
              lastPowerOnTurnoutAddr, dataModel_->accessoryRailProtocol,
              static_cast<RR32Can::TurnoutDirection>(lastPowerOnDirection), request.getPower());
        } else {
          printf("PowerOff for wrong decoder.\n");
        }
      }

      // Send to LocoNet
      LocoNet.requestSwitch(RR32Can::HumanTurnoutAddress(lastPowerOnTurnoutAddr).value(),
                            request.getPower(), request.getDirection());
    }

    // Process CAN: Done through callbacks.

    // Process LocoNet
    lnMsg* LnPacket = LocoNet.receive();
    if (LnPacket) {
      printLnPacket(LnPacket);
      LocoNet.processSwitchSensorMessage(LnPacket);
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
  i2cMsg.setDirection(static_cast<std::underlying_type<RR32Can::TurnoutDirection>::type>(dir));
  i2cMsg.makePowerConsistent();

  SendI2CMessage(i2cMsg);
}

}  // namespace RoutingTask
}  // namespace tasks
