#ifndef __TASKS__ROUTINGTASK__CANENGINEDBSTATEMACHINE_H__
#define __TASKS__ROUTINGTASK__CANENGINEDBSTATEMACHINE_H__

#include "tasks/RoutingTask/CanEngineDB.h"
#include "tasks/RoutingTask/StateMachineBase.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class CanEngineDBStateMachine
 */
class CanEngineDBStateMachine : public StateMachineBase {
 public:
  constexpr static const uint8_t kMaxDownloadTries = 10;

  enum class RequestState { IDLE, REQUESTING, DOWNLOADING };

  CanEngineDBStateMachine(CanEngineDB& engineDb, freertossupport::OsTask& parentTask)
      : StateMachineBase(parentTask), engineDb_(engineDb) {}

  void startRequesting() {
    requestState_ = RequestState::REQUESTING;
    tries = 0;
    startTimerImmediate();
  }

  void loop() {
    if (timerExpired_) {
      if (requestState_ == RequestState::REQUESTING) {
        if (tries < kMaxDownloadTries) {
          engineDb_.fetchEngineDB();
          ++tries;
        } else {
          stopRequesting();
        }
      }
      timerExpired_ = false;
    }
  }

  bool isIdle() { return requestState_ == RequestState::IDLE; }

  RequestState GetState() const { return requestState_; }

  void handlePacket(RR32Can::CanFrame& frame) {
    if (!isIdle() && isFirstPacketInConfigDataStream(frame)) {
      requestState_ = RequestState::DOWNLOADING;
    }
  }

  void notifyEngineDBComplete() {
    if (!isIdle()) {
      stopRequesting();
    }
  }

 private:
  RequestState requestState_{RequestState::IDLE};
  uint8_t tries = 0;
  CanEngineDB& engineDb_;

  void stopRequesting() {
    requestState_ = RequestState::IDLE;
    stopTimer();
  }

  bool isFirstPacketInConfigDataStream(RR32Can::CanFrame& frame) {
    return (frame.id.getCommand() == RR32Can::Command::CONFIG_DATA_STREAM) &&
           (frame.data.dlc == 6 || frame.data.dlc == 7);
  }
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__CANENGINEDBSTATEMACHINE_H__
