#ifndef __STATUSINDICATOR_H__
#define __STATUSINDICATOR_H__

#include "IStatusIndicator.h"
#include "OsTimer.h"
#include "hal/LibOpencm3Hal.h"

/*
 * \brief Class StatusIndicator
 */
class StatusIndicator : public freertossupport::TimerCallbackBase, public IStatusIndicator {
 public:
  enum class State { OFF, IDLE, CANDB_DL, ERROR };

  constexpr static TickType_t kCandbDlBlinkPeriod{300};
  constexpr static TickType_t kErrorBlinkPeriod{100};

  void begin(freertossupport::OsTimer timer, hal::LibOpencm3Hal& hal) {
    timer_ = timer;
    hal_ = &hal;

    updateState();
  }

  void setError() override;
  void clearError() override;

  void setCanDbDownload() override;
  void clearCanDbDownload() override;

  void TimerCallback(TimerHandle_t) override;

 private:
  State state_;
  freertossupport::OsTimer timer_;
  hal::LibOpencm3Hal* hal_;

  bool error{false};
  bool canDbDownload{false};

  void updateState() { setState(compileState()); }

  State compileState() const {
    if (error) {
      return State::ERROR;
    } else {
      if (canDbDownload) {
        return State::CANDB_DL;
      } else {
        return State::IDLE;
      }
    }
  }

  void setState(State state);
};

#endif  // __STATUSINDICATOR_H__
