#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include "hal/HalBase.h"

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
    beginGpio();
    beginRtc();
    beginSerial();
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() { loopCan(); }

  void consumeI2cMessage() {
    i2cBytesRead = 0;
    HalBase::consumeI2cMessage();
  }

  /**
   * \brief Send a given message over I2C.
   */
  void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) override;

  static void i2cEvInt(void);

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginClock();
  void beginGpio();
  void beginRtc();
  void beginSerial();
  void beginI2c();
  void beginCan();

  void loopCan();

  static volatile uint_fast8_t i2cBytesRead;
  static volatile uint_fast8_t i2cBytesSent;

  static volatile MarklinI2C::Messages::AccessoryMsg i2cTxMsg;
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
