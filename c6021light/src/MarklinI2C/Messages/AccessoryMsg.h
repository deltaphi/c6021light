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

  constexpr void makePowerConsistent() {
    if ((this->data_ & kDataPowerMask) == 0) {
      this->data_ &= kDataPowerConsistentMask;
    }
  }

  /**
   * Create a message to be sent from the central to a Keyboard.
   *
   * Mostly interesting for Testcases.
   */
  constexpr static AccessoryMsg makeInbound(const RR32Can::MachineTurnoutAddress& address,
                                            const RR32Can::TurnoutDirection direction, bool power) {
    AccessoryMsg msg;
    msg.destination_ = MarklinI2C::kCentralAddr;
    msg.source_ = makeAddressField(address);
    msg.data_ = makeDataField(address, direction, power);
    msg.makePowerConsistent();
    return msg;
  }

  /**
   * Create a message to be received by the Keyboard.
   */
  constexpr static AccessoryMsg makeOutbound(const RR32Can::MachineTurnoutAddress& address,
                                             const RR32Can::TurnoutDirection direction,
                                             bool power) {
    AccessoryMsg msg;
    msg.destination_ = makeAddressField(address);
    msg.source_ = MarklinI2C::kCentralAddr;
    msg.data_ = makeDataField(address, direction, power);
    msg.makePowerConsistent();
    return msg;
  }

  void print() const;

  uint8_t destination_ = 0;
  uint8_t source_ = 0;
  uint8_t data_ = 0;

 private:
  /// Obtian the de-masked decoder output address.
  uint8_t getDecoderOutput() const;

  constexpr uint8_t getSender() const { return (source_ & kSenderAddrMask) >> 1; }

  constexpr static uint8_t makeAddressField(const RR32Can::MachineTurnoutAddress& address) {
    return 0x20 | ((address.value() & (kSenderAddrMask << 3)) >> 3);
  }

  constexpr static uint8_t makeDataField(const RR32Can::MachineTurnoutAddress& address,
                                         const RR32Can::TurnoutDirection direction, bool power) {
    uint8_t data = 0;

    data |= ((address.value() & (kDataUpperAddrMask >> 2)) << 2);
    data |= ((address.value() & (kDataLowerAddrMask >> 1)) << 1);

    if (power) {
      data |= kDataPowerMask;
    }
    if (direction == RR32Can::TurnoutDirection::GREEN) {
      data |= kDataDirMask;
    }
    return data;
  }
};

}  // namespace Messages
}  // namespace MarklinI2C

#endif  // __MARKLINI2C__MESSAGES__ACCESSORYMSG_H__
