#ifndef __HAL_H__
#define __HAL_H__

#include <Arduino.h>

#include "hal/HalBase.h"

namespace hal {

/*
 * \brief Class ArduinoUnoHal
 */
class ArduinoUnoHal : public HalBase {
 public:
  static constexpr const uint8_t kMsgBytesLength =
      MarklinI2C::Messages::AccessoryMsg::kAccesoryMessageBytes - 1;

  struct I2CBuf {
    volatile bool msgValid;
    volatile uint8_t msgBytes[kMsgBytesLength];
  };

  struct TimedI2CBuf : public I2CBuf {
    unsigned long timestamp;  ///< Timestamp then message became valid in Microseconds
  };

  ///
  void begin(uint8_t i2caddr) {
    HalBase::begin(i2caddr);
    Serial.begin(115200);
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() { loopCan(); }

  void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) override;

  bool i2cAvailable() const { return i2cRxBuf.msgValid; }
  MarklinI2C::Messages::AccessoryMsg getI2cMessage() const;
  virtual void consumeI2cMessage() { i2cRxBuf.msgValid = false; }

 private:
  /// The last message that was received over i2c.
  static TimedI2CBuf i2cRxBuf;

  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginI2c();
  void beginCan();

  void loopCan();

  static void receiveEvent(int howMany);
};

}  // namespace hal

#endif  // __HAL_H__
