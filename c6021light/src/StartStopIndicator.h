#ifndef __STARTSTOPINDICATOR_H__
#define __STARTSTOPINDICATOR_H__

#include "RR32Can/callback/SystemCbk.h"

#include "OsTimer.h"
#include "hal/LibOpencm3Hal.h"

/*
 * \brief Class StartStopIndicator
 */
class StartStopIndicator : public freertossupport::TimerCallbackBase,
                           public RR32Can::callback::SystemCbk {
 public:
  enum class State { STOP, START, UNKNOWN };

  constexpr static TickType_t kUnknownBlinkPeriod{100};

  void begin(freertossupport::OsTimer timer, hal::LibOpencm3Hal& hal) {
    timer_ = timer;
    hal_ = &hal;

    setState(State::UNKNOWN);
  }

  void setSystemState(bool onOff, bool response) override;
  void setUnkown();

  void TimerCallback(TimerHandle_t) override;

 private:
  State state_;
  freertossupport::OsTimer timer_;
  hal::LibOpencm3Hal* hal_;

  void setState(State state);
};

#endif  // __STARTSTOPINDICATOR_H__
