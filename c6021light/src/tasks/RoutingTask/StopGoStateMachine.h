#ifndef __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__
#define __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__

#include "CANForwarder.h"
#include "RR32Can/util/constexpr.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class StopGoStateMachine
 */
class StopGoStateMachine {
 public:
  enum class State { IDLE, REQUESTING };

  StopGoStateMachine(tasks::RoutingTask::CANForwarder& canForwarder)
      : canForwarder_(canForwarder) {}

  void startRequesting() {
    state_ = State::REQUESTING;
    timerExpired = false;
  }

  void notifyExpiry() { timerExpired = true; }

  void loop() {
    if (timerExpired) {
      if (state_ == State::REQUESTING) {
        sendRequest();
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
  bool timerExpired = false;
  CANForwarder& canForwarder_;

  static bool isSystemStopGoSubcommand(const RR32Can::SystemMessage message) {
    const RR32Can::SystemSubcommand subcommand = message.getSubcommand();
    return subcommand == RR32Can::SystemSubcommand::SYSTEM_GO ||
           subcommand == RR32Can::SystemSubcommand::SYSTEM_STOP ||
           subcommand == RR32Can::SystemSubcommand::SYSTEM_HALT;
  }

  void notifyStatusMessageReceived() { state_ = State::IDLE; }
  void sendRequest() { canForwarder_.forward(RR32Can::util::System_GetStatus()); }
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__STOPGOSTATEMACHINE_H__
