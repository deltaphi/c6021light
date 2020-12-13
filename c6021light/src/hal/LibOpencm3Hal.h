#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include <atomic>

#include "hal/HalBase.h"

#include "OsQueue.h"

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal : public HalBase {
 public:
  void begin(ConsoleManager* console) {
    HalBase::begin(console);
    beginClock();
    beginGpio();
    beginSerial();
    beginLocoNet();
    beginEE();
  }

  void loopSerial();

  void led(bool on) override;
  void toggleLed() override;

  void SaveConfig(const DataModel& dataModel) override;
  DataModel LoadConfig() override;

 private:
  void beginClock();
  void beginGpio();
  void beginSerial();
  void beginLocoNet();
  void beginEE();
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
