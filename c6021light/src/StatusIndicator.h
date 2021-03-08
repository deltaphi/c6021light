#ifndef __STATUSINDICATOR_H__
#define __STATUSINDICATOR_H__

#include "OsTimer.h"
#include "hal/LibOpencm3Hal.h"

/*
 * \brief Class StatusIndicator
 */
class StatusIndicator : public freertossupport::TimerCallbackBase {
 public:
  enum class State { OFF, IDLE, CANDB_DL, ERROR };

  constexpr static TickType_t kCandbDlBlinkPeriod{300};
  constexpr static TickType_t kErrorBlinkPeriod{100};

  void begin(freertossupport::OsTimer timer, hal::LibOpencm3Hal& hal) {
    timer_ = timer;
    hal_ = &hal;

    setState(State::OFF);
  }

  void setState(State state);

  void TimerCallback(TimerHandle_t) override;

 private:
  State state_;
  freertossupport::OsTimer timer_;
  hal::LibOpencm3Hal* hal_;
};

#endif  // __STATUSINDICATOR_H__
