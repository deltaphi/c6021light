#ifndef __MOCKS__KEYBOARDMOCK_H__
#define __MOCKS__KEYBOARDMOCK_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"

namespace mocks {

inline MarklinI2C::Messages::AccessoryMsg makeReceivedAccessoryMsg(
    const RR32Can::MachineTurnoutAddress& address, const RR32Can::TurnoutDirection direction,
    bool power) {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.destination_ = 0x7f;
  msg.source_ = 0x20 | ((address.value() & 0xF0) >> 3);

  msg.data_ |= ((address.value() & 0b00001100) << 2);
  msg.data_ |= ((address.value() & 0b00000011) << 1);

  if (power) {
    msg.data_ |= 0x08;
  }
  if (direction == RR32Can::TurnoutDirection::GREEN) {
    msg.data_ |= 1;
  }
  msg.makePowerConsistent();

  return msg;
}

}  // namespace mocks

#endif  // __MOCKS__KEYBOARDMOCK_H__
