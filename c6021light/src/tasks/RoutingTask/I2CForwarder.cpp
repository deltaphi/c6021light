#include "tasks/RoutingTask/I2CForwarder.h"

#include "RR32Can/messages/TurnoutPacket.h"

#include "DataModel.h"

#include "hal/stm32I2C.h"

#include <cstdio>

namespace tasks {
namespace RoutingTask {

/**
 * \brief Two addresses are on the same decoder, if they match apart from
 * the lowest two bits.
 */
namespace {
bool sameDecoder(const RR32Can::MachineTurnoutAddress left,
                 const RR32Can::MachineTurnoutAddress right) {
  constexpr const uint8_t mask = 0xFC;
  return (left.value() & mask) == (right.value() & mask);
}
}  // namespace

void I2CForwarder::forward(const RR32Can::CanFrame& frame) {
  switch (frame.id.getCommand()) {
    case RR32Can::Command::ACCESSORY_SWITCH: {
      const RR32Can::TurnoutPacket turnoutPacket(const_cast<RR32Can::Data&>(frame.data));
      if (frame.id.isResponse()) {
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

void I2CForwarder::forwardLocoChange(const RR32Can::LocomotiveData&, LocoDiff_t&) {
  // Currently no loco control on I2C
}

MarklinI2C::Messages::AccessoryMsg I2CForwarder::prepareI2cMessage() {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.source_ = DataModel::kMyAddr;
  return msg;
}

void I2CForwarder::SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg) {
  printf("I2C TX: ");
  msg.print();

  hal::I2CBuf buf;
  buf.msgBytes[0] = msg.destination_ >> 1;
  buf.msgBytes[1] = msg.data_;

  hal::sendI2CMessage(buf);
}

bool I2CForwarder::MakeRR32CanMsg(const MarklinI2C::Messages::AccessoryMsg& request,
                                  RR32Can::CanFrame& frame) {
  frame.id.setCommand(RR32Can::Command::ACCESSORY_SWITCH);
  frame.id.setResponse(false);

  RR32Can::TurnoutPacket turnoutPacket(frame.data);
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
    if (sameDecoder(i2cAddr, lastPowerOnTurnoutAddr)) {
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

}  // namespace RoutingTask
}  // namespace tasks
