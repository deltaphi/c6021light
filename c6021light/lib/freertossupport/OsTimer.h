#ifndef __FREERTOSSUPPORT__OSTIMER_H__
#define __FREERTOSSUPPORT__OSTIMER_H__

#include <type_traits>

#include "FreeRTOS.h"
#include "timers.h"

namespace freertossupport {

/*
 * \brief Class OsTimer
 */
class OsTimer {
 public:
  void Start() { xTimerStart(handle_, 0); }
  void Stop() { xTimerStop(handle_, 0); }

  bool Running() const { return (xTimerIsTimerActive(handle_) != pdFALSE); }

  TimerHandle_t getHandle() { return handle_; }

 protected:
  TimerHandle_t handle_;
};

class TimerCallbackBase {
 public:
  virtual void TimerCallback(TimerHandle_t) = 0;
};

class StaticOsTimer : public OsTimer {
 public:
  void Create(const char* name, uint32_t period_ms, bool autoReload, TimerCallbackBase* callback) {
    this->handle_ =
        xTimerCreateStatic(name, pdMS_TO_TICKS(period_ms), (autoReload ? pdTRUE : pdFALSE),
                           callback, timerCallbackHelper, &tcb);
    configASSERT(this->handle_ != NULL);
  }

 private:
  static void timerCallbackHelper(TimerHandle_t handle);

  StaticTimer_t tcb;
};

}  // namespace freertossupport

#endif  // __FREERTOSSUPPORT__OSTIMER_H__
