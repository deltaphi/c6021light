#ifndef __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__
#define __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__

#include <atomic>

#include "OsTask.h"
#include "OsTimer.h"

#include "CANForwarder.h"
#include "RR32Can/util/constexpr.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class StopGoStateMachine
 */
class StopGoStateMachine : public freertossupport::TimerCallbackBase {
 public:
  enum class State { IDLE, REQUESTING };
  constexpr static const uint8_t kMaxRetries = 10;

  StopGoStateMachine(tasks::RoutingTask::CANForwarder& canForwarder,
                     freertossupport::OsTask& parentTask)
      : canForwarder_(canForwarder), parentTask_(parentTask) {}

  void setTimer(freertossupport::OsTimer& timer) { timer_ = &timer; }

  void startRequesting() {
    state_ = State::REQUESTING;
    tries = 0;
    timerExpired = true;
    if (timer_ != nullptr) {
      timer_->Start();
    }
  }

  void TimerCallback(TimerHandle_t) override {
    timerExpired = true;
    // Actual handling performed in loop() which is called by parentTask.
    parentTask_.notify();
  }

  void loop() {
    if (timerExpired) {
      if (state_ == State::REQUESTING) {
        if (tries < kMaxRetries) {
          sendRequest();
          ++tries;
        } else {
          stopRequesting();
        }
      }
      timerExpired = false;
    }
  }

  bool isIdle() const { return state_ == State::IDLE; }

  void handlePacket(const RR32Can::CanFrame& frame) {
    if (frame.id.getCommand() == RR32Can::Command::SYSTEM_COMMAND) {
      if (isSystemStopGoSubcommand(const_cast<RR32Can::CanFrame&>(frame).data)) {
        notifyStatusMessageReceived();
      }
    }
  }

 private:
  State state_ = State::IDLE;
  uint8_t tries;
  std::atomic_bool timerExpired{false};
  CANForwarder& canForwarder_;
  freertossupport::OsTask& parentTask_;
  freertossupport::OsTimer* timer_{nullptr};

  static bool isSystemStopGoSubcommand(const RR32Can::SystemMessage message) {
    const RR32Can::SystemSubcommand subcommand = message.getSubcommand();
    return subcommand == RR32Can::SystemSubcommand::SYSTEM_GO ||
           subcommand == RR32Can::SystemSubcommand::SYSTEM_STOP ||
           subcommand == RR32Can::SystemSubcommand::SYSTEM_HALT;
  }

  void stopRequesting() {
    state_ = State::IDLE;
    if (timer_ != nullptr) {
      timer_->Stop();
    }
  }

  void notifyStatusMessageReceived() {
    if (state_ != State::IDLE) {
      stopRequesting();
    }
  }

  void sendRequest() { canForwarder_.forward(RR32Can::util::System_GetStatus()); }
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__
