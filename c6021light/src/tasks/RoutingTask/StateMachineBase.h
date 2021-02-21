#ifndef C6021LIGHT_STATEMACHINEBASE_H
#define C6021LIGHT_STATEMACHINEBASE_H

#include <atomic>

#include "OsTask.h"
#include "OsTimer.h"

namespace tasks {
namespace RoutingTask {

class StateMachineBase : public freertossupport::TimerCallbackBase {
 public:
  StateMachineBase(freertossupport::OsTask& parentTask) : parentTask_(parentTask) {}

  void setTimer(freertossupport::OsTimer& timer) { timer_ = &timer; }

  void TimerCallback(TimerHandle_t) final {
    timerExpired_ = true;
    // Actual handling performed in loop() which is called by parentTask.
    parentTask_.notify();
  }

 protected:
  std::atomic_bool timerExpired_{false};

  void startTimer() {
    if (timer_ != nullptr) {
      timer_->Start();
    }
  }

  void stopTimer() {
    if (timer_ != nullptr) {
      timer_->Stop();
    }
  }

 private:
  freertossupport::OsTask& parentTask_;
  freertossupport::OsTimer* timer_{nullptr};
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // C6021LIGHT_STATEMACHINEBASE_H
