#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include "hal/Led.h"

#include "c6021lightConfig.h"

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
    beginXpressNet();
  }

  Led<kStartStopLEDBank, kStartStopLEDPin> startStopLed;
  Led<kStatusLEDBank, kStatusLEDPin> statusLed;

 private:
  void beginClock();
  void beginGpio();
  void beginLocoNet();
  void beginXpressNet();
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
