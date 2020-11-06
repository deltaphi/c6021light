#include "MarklinI2C/Messages/AccessoryMsg.h"

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

uint8_t AccessoryMsg::getTurnoutAddr() const {
  uint8_t addr = 0;
  addr |= getSender();
  addr <<= 4;
  addr |= getDecoderOutput();
  return addr;
}

void AccessoryMsg::setTurnoutAddr(uint8_t addr) {
  {
    this->destination_ = addr & 0xF0;
    this->destination_ >>= 3;
    this->destination_ &= 0b00100000;
  }
  {
    uint8_t decoderAddrPart = addr & 0x0C;
    decoderAddrPart <<= 2;
    decoderAddrPart &= kDataUpperAddrMask;
    this->data_ |= decoderAddrPart;
  }
  {
    uint8_t decoderOutput = addr & 0x03;
    decoderOutput <<= 1;
    decoderOutput &= kDataLowerAddrMask;
    this->data_ |= decoderOutput;
  }
}

AccessoryMsg AccessoryMsg::makeResponse() const {
  AccessoryMsg response;
  response.destination_ = source_ >> 1;  // Shift Source address to destination space.
  response.source_ = destination_ << 1;  // Shift destination address to source space.
  response.data_ = data_;
  return response;
}

void AccessoryMsg::print() const {
#ifdef ARDUINO
  // Actual output currently available on Arduino only.
  Serial.print('[');
  Serial.print(destination_, BIN);
  Serial.print(' ');
  Serial.print(source_, BIN);
  Serial.print(' ');
  Serial.print(data_, BIN);
  Serial.print(']');

  // Sender
  Serial.print(F(" Keyboard: "));
  Serial.print(getSender(), DEC);

  Serial.print(F(" Decoder: "));
  Serial.print(getDecoderOutput(), DEC);

  Serial.print(F(" (Turnout Addr: "));
  Serial.print(getTurnoutAddr(), DEC);

  Serial.print(F(") Direction: "));
  switch (getDirection()) {
    case kDataDirRed:
      Serial.print(F("RED  "));
      break;
    case kDataDirGreen:
      Serial.print(F("GREEN"));
      break;
    default:
      Serial.print(F("WTF"));
      break;
  }

  Serial.print(F(" Power: "));
  switch (getPower()) {
    case 0:
      Serial.print(F("OFF"));
      break;
    case 1:
      Serial.print(F("ON "));
      break;
    default:
      Serial.print(F("WTF"));
      break;
  }

  Serial.println();
#endif
}

}  // namespace Messages
}  // namespace MarklinI2C
