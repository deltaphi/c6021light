#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include <atomic>

#include "DataModel.h"

#include "OsQueue.h"

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal {
 public:
  void begin() {
    beginClock();
    beginGpio();
    beginLocoNet();
    beginEE();
  }

  void led(bool on);
  void toggleLed();

  void SaveConfig(const DataModel& dataModel);
  DataModel LoadConfig();

 private:
  void beginClock();
  void beginGpio();
  void beginLocoNet();
  void beginEE();
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
