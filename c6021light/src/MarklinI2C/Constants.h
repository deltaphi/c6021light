#ifndef __MARKLINI2C__CONSTANTS_H__
#define __MARKLINI2C__CONSTANTS_H__

#include <cstdint>

namespace MarklinI2C {
/// Maximum length of a message in bytes.
constexpr const uint8_t kMessageMaxBytes = 4;
constexpr const uint8_t kSenderByte = 0;
constexpr const uint8_t kDestinationByte = 1;

constexpr const uint8_t kAccessoryMessageLength = 3;
constexpr const uint8_t kLocoMessageLength = 3;
constexpr const uint8_t kFunctionMessageLength = kLocoMessageLength;


/// I2C Address of the central.
constexpr const uint8_t kCentralAddr = 0x7F;

/// Bitmask containing sender bits.
constexpr const uint8_t kSenderAddrMask = 0b00011110;

/// Bitmask containing the message type bits.
constexpr const uint8_t kMsgTypeMask = 0b01100000;
constexpr const uint8_t kMsgTypeLoco = 0b00000000;
constexpr const uint8_t kMsgTypeAccessory = 0b00100000;
constexpr const uint8_t kMsgTypeFunction = 0b01000000;

/// Accessory Data: The direction bit.
constexpr const uint8_t kDataDirMask = 0x01;
constexpr const uint8_t kDataDirRed = 0x00;
constexpr const uint8_t kDataDirGreen = 0x01;

/// Accessory Data: The power on/off bit.
constexpr const uint8_t kDataPowerMask = 0b00001000;

/// Accessory Data: The turnout address lower bits.
constexpr const uint8_t kDataLowerAddrMask = 0b00000110;
constexpr const uint8_t kDataUpperAddrMask = 0b00110000;

}  // namespace MarklinI2C

#endif  // __MARKLINI2C__CONSTANTS_H__
