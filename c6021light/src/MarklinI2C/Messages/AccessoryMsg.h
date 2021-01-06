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

  AccessoryMsg(){};

  AccessoryMsg(const AccessoryMsg&) = default;
  AccessoryMsg& operator=(const AccessoryMsg&) = default;

  AccessoryMsg(AccessoryMsg&&) = default;
  AccessoryMsg& operator=(AccessoryMsg&&) = default;

  AccessoryMsg(uint8_t destination, uint8_t source, uint8_t data)
      : destination_(destination), source_(source), data_(data){};

  bool operator==(const AccessoryMsg& other) const {
    return this->destination_ == other.destination_ && this->source_ == other.source_ &&
           this->data_ == other.data_;
  }

  uint8_t getSender() const { return (source_ & 0b00011110) >> 1; }

  /// Obtian the de-masked decoder output address.
  uint8_t getDecoderOutput() const;

  /**
   * \brief Obtain the complete turnout address.
   *
   * The result is 0-based. Pressing a button for Turnout 1
   * on the first Keyboard is Address 0.
   */
  RR32Can::MachineTurnoutAddress getTurnoutAddr() const;

  RR32Can::TurnoutDirection getDirection() const {
    return (((data_ & kDataDirMask) == 0) ? RR32Can::TurnoutDirection::RED
                                          : RR32Can::TurnoutDirection::GREEN);
  }
  uint8_t getPower() const { return (data_ & kDataPowerMask) >> 3; }

  void setDirection(RR32Can::TurnoutDirection direction) {
    if (direction == RR32Can::TurnoutDirection::RED) {
      this->data_ &= 0xFE;
    } else {
      this->data_ |= kDataDirMask;
    }
  }

  void setPower(uint8_t power) {
    power <<= 3;
    power &= kDataPowerMask;
    this->data_ |= power;
  }

  void makePowerConsistent() {
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
  AccessoryMsg makeResponse() const;

  uint8_t destination_ = 0;
  uint8_t source_ = 0;
  uint8_t data_ = 0;
};

}  // namespace Messages
}  // namespace MarklinI2C

#endif  // __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__
