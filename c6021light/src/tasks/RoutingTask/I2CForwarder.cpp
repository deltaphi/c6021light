#include "tasks/RoutingTask/I2CForwarder.h"

#include <cstdio>

#include "RR32Can/messages/TurnoutPacket.h"

#include "DataModel.h"

#include "hal/stm32I2C.h"

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
        auto i2cMsg = MarklinI2C::Messages::AccessoryMsg::makeOutbound(
            turnoutAddr, turnoutPacket.getDirection(), turnoutPacket.getPower());

        printf("I2C TX: ");
        i2cMsg.print();
        hal::sendI2CMessage(i2cMsg);
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

void I2CForwarder::sendI2CResponseIfEnabled(const MarklinI2C::Messages::AccessoryMsg& i2cMsg) {
  if (dataModel_->generateI2CTurnoutResponse) {
    const auto response = MarklinI2C::Messages::AccessoryMsg::makeOutbound(i2cMsg);
    hal::sendI2CMessage(response);
  }
}

}  // namespace RoutingTask
}  // namespace tasks
