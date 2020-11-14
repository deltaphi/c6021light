#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include <atomic>

#include "hal/HalBase.h"

#include "hal/stm32I2C.h"

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
    beginSerial();
    beginI2C(i2caddr);
    beginCan();
    beginEE();
  }

  /// Receive Packet from CAN and forward to station.
  void loop() {
    loopCan();
    loopSerial();
  }

  void led(bool on) override;
  void toggleLed() override;

  static void i2cEvInt(void);

  void SaveConfig(const DataModel& dataModel) override;
  DataModel LoadConfig() override;

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginClock();
  void beginGpio();
  void beginSerial();
  void beginCan();
  void beginEE();

  void loopCan();
  void loopSerial();
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
