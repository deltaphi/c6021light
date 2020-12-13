#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include <atomic>

#include "ConsoleManager.h"
#include "DataModel.h"

#include "OsQueue.h"

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal {
 public:
  void begin(ConsoleManager* console) {
    console_ = console;
    beginClock();
    beginGpio();
    beginSerial();
    beginLocoNet();
    beginEE();
  }

  void loopSerial();

  void led(bool on);
  void toggleLed();

  void SaveConfig(const DataModel& dataModel);
  DataModel LoadConfig();

 private:
  void beginClock();
  void beginGpio();
  void beginSerial();
  void beginLocoNet();
  void beginEE();

  ConsoleManager* console_;
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
