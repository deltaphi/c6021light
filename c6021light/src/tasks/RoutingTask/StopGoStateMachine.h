#ifndef __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__
#define __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__

#include "OsTask.h"
#include "OsTimer.h"

#include "CANForwarder.h"
#include "RR32Can/util/constexpr.h"
#include "StateMachineBase.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class StopGoStateMachine
 */
class StopGoStateMachine : public StateMachineBase {
 public:
  enum class State { IDLE, REQUESTING };
  constexpr static const uint8_t kMaxRetries = 10;

  StopGoStateMachine(tasks::RoutingTask::CANForwarder& canForwarder,
                     freertossupport::OsTask& parentTask)
      : StateMachineBase(parentTask), canForwarder_(canForwarder) {}

  void startRequesting() {
    state_ = State::REQUESTING;
    tries = 0;
    startTimerImmediate();
  }

  void loop() {
    if (timerExpired_) {
      if (state_ == State::REQUESTING) {
        if (tries < kMaxRetries) {
          sendRequest();
          ++tries;
        } else {
          stopRequesting();
        }
      }
      timerExpired_ = false;
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
  CANForwarder& canForwarder_;

  static bool isSystemStopGoSubcommand(const RR32Can::SystemMessage message) {
    const RR32Can::SystemSubcommand subcommand = message.getSubcommand();
    return subcommand == RR32Can::SystemSubcommand::SYSTEM_GO ||
           subcommand == RR32Can::SystemSubcommand::SYSTEM_STOP ||
           subcommand == RR32Can::SystemSubcommand::SYSTEM_HALT;
  }

  void stopRequesting() {
    state_ = State::IDLE;
    stopTimer();
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
