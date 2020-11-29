#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include <atomic>

#include "hal/HalBase.h"
#include "hal/stm32I2C.h"

#include "OsQueue.h"

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
  void begin(uint8_t i2caddr, ConsoleManager* console, xTaskHandle routingTaskHandle) {
    HalBase::begin(i2caddr, console);
    this->taskToNotify = routingTaskHandle;
    beginClock();
    beginGpio();
    beginSerial();
    beginI2C(i2caddr, routingTaskHandle);
    beginCan();
    beginEE();
  }

  void loopCan();
  void loopSerial();

  void led(bool on) override;
  void toggleLed() override;

  void SaveConfig(const DataModel& dataModel) override;
  DataModel LoadConfig() override;

  using CanQueueType = freertossupport::OsQueue<LibOpencm3Hal::CanMsg>;

  static CanQueueType canrxq;
  static TaskHandle_t taskToNotify;

 private:
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) override;

  void beginClock();
  void beginGpio();
  void beginSerial();
  void beginCan();
  void beginEE();
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
