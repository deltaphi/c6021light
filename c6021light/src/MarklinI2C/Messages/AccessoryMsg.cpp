#include "MarklinI2C/Messages/AccessoryMsg.h"

#include <cinttypes>

#include "RR32Can/StlAdapter.h"

namespace MarklinI2C {
namespace Messages {

uint8_t AccessoryMsg::getDecoderOutput() const {
  uint8_t lowerBits = data_ & kDataLowerAddrMask;
  lowerBits >>= 1;

  uint8_t upperBits = data_ & kDataUpperAddrMask;
  upperBits >>= 2;

  uint8_t addr = upperBits | lowerBits;
  return addr;
}

RR32Can::MachineTurnoutAddress AccessoryMsg::getTurnoutAddr(const uint8_t keyboardAddr) const {
  uint8_t addr = 0;
  addr |= keyboardAddr;
  addr <<= 4;
  addr |= getDecoderOutput();
  return RR32Can::MachineTurnoutAddress(addr);
}

void AccessoryMsg::print(bool isRx) const {
  const char* directionString;
  const char* powerString;

  switch (getDirection()) {
    case RR32Can::TurnoutDirection::RED:
      directionString = "RED  ";
      break;
    case RR32Can::TurnoutDirection::GREEN:
      directionString = "GREEN";
      break;
    default:
      directionString = "WTF  ";
      break;
  }

  switch (getPower()) {
    case 0:
      powerString = "OFF";
      break;
    case 1:
      powerString = "ON ";
      break;
    default:
      powerString = "WTF";
      break;
  }

  constexpr static const char* formatString =
      "[0x%x, 0x%x, 0x%x] - Keyboard: %i, Decoder: %i, (Turnout: %" PRId32
      ", Direction: %s, Power: %s)\n";
  if (isRx) {
    printf(formatString, destination_, source_, data_, getInboundSender(), getDecoderOutput(),
           getInboundTurnoutAddr().value(), directionString, powerString);
  } else {
    printf(formatString, destination_, source_, data_, getOutboundDestination(), getDecoderOutput(),
           getOutboundTurnoutAddr().value(), directionString, powerString);
  }
}

}  // namespace Messages
}  // namespace MarklinI2C
