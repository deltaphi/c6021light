#ifndef __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__
#define __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#endif

#include "MarklinI2C/Constants.h"
#include "RR32Can/Types.h"

namespace MarklinI2C {
namespace Messages {

/**
 * \brief Class representing an accessory switch packet on I2C.
 */
class AccessoryMsg {
 public:
  constexpr static const uint8_t kAccesoryMessageBytes = 3;

  constexpr AccessoryMsg(){};

  constexpr AccessoryMsg(const AccessoryMsg&) = default;
  constexpr AccessoryMsg& operator=(const AccessoryMsg&) = default;

  constexpr AccessoryMsg(AccessoryMsg&&) = default;
  constexpr AccessoryMsg& operator=(AccessoryMsg&&) = default;

  constexpr AccessoryMsg(uint8_t destination, uint8_t source, uint8_t data)
      : destination_(destination), source_(source), data_(data){};

  constexpr bool operator==(const AccessoryMsg& other) const {
    return this->destination_ == other.destination_ && this->source_ == other.source_ &&
           this->data_ == other.data_;
  }

  constexpr uint8_t getSender() const { return (source_ & 0b00011110) >> 1; }

  /// Obtian the de-masked decoder output address.
  uint8_t getDecoderOutput() const;

  /**
   * \brief Obtain the complete turnout address.
   *
   * The result is 0-based. Pressing a button for Turnout 1
   * on the first Keyboard is Address 0.
   */
  RR32Can::MachineTurnoutAddress getTurnoutAddr() const;

  constexpr RR32Can::TurnoutDirection getDirection() const {
    return (((data_ & kDataDirMask) == 0) ? RR32Can::TurnoutDirection::RED
                                          : RR32Can::TurnoutDirection::GREEN);
  }
  constexpr uint8_t getPower() const { return (data_ & kDataPowerMask) >> 3; }

  constexpr void setDirection(RR32Can::TurnoutDirection direction) {
    if (direction == RR32Can::TurnoutDirection::RED) {
      this->data_ &= 0xFE;
    } else {
      this->data_ |= kDataDirMask;
    }
  }

  constexpr void setPower(uint8_t power) {
    power <<= 3;
    power &= kDataPowerMask;
    this->data_ |= power;
  }

  constexpr void makePowerConsistent() {
    if ((this->data_ & kDataPowerMask) == 0) {
      this->data_ &= 0xF0;
    }
  }

  void setTurnoutAddr(RR32Can::MachineTurnoutAddress addr);

  void print() const;

  /**
   * \brief Assume that this message is a request message from a keyboard and
   * create a response message for the keyboard.
   */
  constexpr AccessoryMsg makeResponse() {
    AccessoryMsg response;
    response.destination_ = source_ >> 1;  // Shift Source address to destination space.
    response.source_ = destination_ << 1;  // Shift destination address to source space.
    response.data_ = data_;
    return response;
  }

  uint8_t destination_ = 0;
  uint8_t source_ = 0;
  uint8_t data_ = 0;
};

inline constexpr MarklinI2C::Messages::AccessoryMsg makeInboundAccessoryMsg(
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

inline constexpr MarklinI2C::Messages::AccessoryMsg makeOutboundAccessoryMsg(
    const RR32Can::MachineTurnoutAddress& address, const RR32Can::TurnoutDirection direction,
    bool power) {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.source_ = 0x7f;
  msg.destination_ = 0x20 | ((address.value() & 0xF0) >> 3);

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

}  // namespace Messages
}  // namespace MarklinI2C

#endif  // __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__
