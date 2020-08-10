#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include <atomic>

#include "hal/HalBase.h"

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal : public HalBase {
 public:
  static constexpr const uint_fast8_t kMsgBytesLength =
      MarklinI2C::Messages::AccessoryMsg::kAccesoryMessageBytes - 1;

  typedef struct {
    // RR32Can::Identifier id;
    uint32_t id;
    RR32Can::Data data;
  } CanMsg;

  struct I2CBuf {
    uint_fast8_t bytesProcessed;
    std::atomic_bool msgValid;
    volatile uint8_t msgBytes[kMsgBytesLength];
  };

  ///
  void begin(uint8_t i2caddr, ConsoleManager* console) {
    HalBase::begin(i2caddr, console);
    beginClock();
    beginGpio();
    beginRtc();
    beginSerial();
    beginI2c();
    beginCan();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() {
    loopCan();
    loopSerial();
  }

  bool i2cAvailable() const { return i2cRxBuf.msgValid.load(std::memory_order_acquire); }
  MarklinI2C::Messages::AccessoryMsg getI2cMessage() const;

  /**
   * \brief Send a given message over I2C.
   */
  void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) override;

  void led(bool on) override;
  void toggleLed() override;

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
  void loopSerial();

  static I2CBuf i2cRxBuf;
  static I2CBuf i2cTxBuf;
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
