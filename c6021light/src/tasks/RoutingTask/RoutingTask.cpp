#include "tasks/RoutingTask/RoutingTask.h"

#include <atomic>
#include <cstdint>
#include <cstdio>

#include "RR32Can/RR32Can.h"
#include "hal/stm32I2C.h"

#include "OsQueue.h"

namespace tasks {
namespace RoutingTask {

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
  hal::triggerI2cTx();
}

void RoutingTask::OnAccessoryPacket(RR32Can::TurnoutPacket& packet, bool response) {
  if (!response) {
    printf(" Ignoring Accessory request packet");
    return;
  }

  printf(" Got an Accessory packet!");

  if ((packet.locid & RR32Can::kMMAccessoryAddrStart) != RR32Can::kMMAccessoryAddrStart) {
    // Not an MM2 packet
    return;
  }

  uint16_t turnoutAddr = packet.locid & 0x03FF;
  if (turnoutAddr > 0xFF) {
    // Addr too large for the i2c bus.
    return;
  }

  // Convert to i2c confirmation packet
  MarklinI2C::Messages::AccessoryMsg i2cMsg = RoutingTask::prepareI2cMessage();

  i2cMsg.setTurnoutAddr(turnoutAddr);
  i2cMsg.setPower(packet.power);
  i2cMsg.setDirection(packet.position);

  RoutingTask::SendI2CMessage(i2cMsg);
}

/**
 * \brief When a message was received, create and send a response message.
 */
void RoutingTask::main() {
  while (1) {
    halImpl_->loop();

    // Process I2C
    hal::I2CQueueType::ReceiveResult receiveResult = hal::i2cRxQueue.Receive(0);
    if (receiveResult.errorCode == pdPASS) {
      MarklinI2C::Messages::AccessoryMsg request = getI2CMessage(receiveResult.element);
      request.print();
      // If this is a power ON packet: Send directly to CAN
      if (request.getPower()) {
        lastPowerOnDirection = request.getDirection();
        lastPowerOnTurnoutAddr = request.getTurnoutAddr();
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
        if (sameDecoder(i2cAddr, lastPowerOnTurnoutAddr)) {
          RR32Can::RR32Can.SendAccessoryPacket(
              lastPowerOnTurnoutAddr, dataModel_->accessoryRailProtocol,
              static_cast<RR32Can::TurnoutDirection>(lastPowerOnDirection), request.getPower());
        }
      }
    }
    // Process CAN: Done through callbacks.
  }
}

}  // namespace RoutingTask
}  // namespace tasks

void routingTaskMain(void* args) { static_cast<tasks::RoutingTask::RoutingTask*>(args)->main(); }
