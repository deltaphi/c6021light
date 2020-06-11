#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include "hal/HalBase.h"

/// Give the current RTC value in seconds.
unsigned long seconds();

/// Give the current RTC value in milliseconds.
unsigned long millis();

/// Give the current RTC value in microseconds.
unsigned long micros();

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal : public HalBase {
 public:
  typedef struct {
    // RR32Can::Identifier id;
    uint32_t id;
    RR32Can::Data data;
  } CanMsg;

  ///
  void begin(uint8_t i2caddr) {
    HalBase::begin(i2caddr);
    beginClock();
    beginRtc();
    beginSerial();
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() { loopCan(); }

  void consumeI2cMessage() {
    bytesRead = 0;
    HalBase::consumeI2cMessage();
  }

  /**
   * \brief Send a given message over I2C.
   */
  void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) override;

  static void i2cEvInt(void);
  static void canRxInt(void);

  static volatile bool canAvailable;

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginClock();
  void beginRtc();
  void beginSerial();
  void beginI2c();
  void beginCan();

  void loopCan();

  static volatile uint_fast8_t bytesRead;
  static volatile uint_fast8_t bytesSent;

  static volatile MarklinI2C::Messages::AccessoryMsg i2cTxMsg;

  static volatile CanMsg canRxMsg;
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
