#ifndef __TASKS__ROUTINGTASK__CANENGINEDBSTATEMACHINE_H__
#define __TASKS__ROUTINGTASK__CANENGINEDBSTATEMACHINE_H__

#include "tasks/RoutingTask/CanEngineDB.h"
#include "tasks/RoutingTask/StateMachineBase.h"

#include "IStatusIndicator.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class CanEngineDBStateMachine
 */
class CanEngineDBStateMachine : public StateMachineBase {
 public:
  constexpr static const uint8_t kMaxDownloadTries = 10;
  constexpr static const uint8_t kDownloadTimeout =
      3;  // After this many timer expiries in the download state without incoming messages, start
          // another request.

  enum class RequestState { IDLE, REQUESTING, DOWNLOADING };

  CanEngineDBStateMachine(CanEngineDB& engineDb, freertossupport::OsTask& parentTask)
      : StateMachineBase(parentTask), engineDb_(engineDb) {}

  void startRequesting() {
    requestState_ = RequestState::REQUESTING;
    printf("CanEngineDBStateMachine: startRequesting()\n");
    if (statusIndicator != nullptr) {
      statusIndicator->setCanDbDownload();
    }
    tries = 0;
    startTimerImmediate();
  }

  void loop() {
    if (timerExpired_) {
      printf("CanEngineDBStateMachine: Timer! ");
      if (requestState_ == RequestState::DOWNLOADING) {
        ++downloadTime;
        if (downloadTime > kDownloadTimeout) {
          printf("Download Timeout expired. ");
          requestState_ = RequestState::REQUESTING;
        }
      }

      if (requestState_ == RequestState::REQUESTING) {
        if (tries < kMaxDownloadTries) {
          printf("Requesting try %d. ", tries);
          RR32Can::RR32Can.AbortCurrentConfigRequest();
          engineDb_.fetchEngineDB();
          ++tries;
        } else {
          printf("Giving up. ");
          stopRequesting();
        }
      }
      printf("\n");
      timerExpired_ = false;
    }
  }

  bool isIdle() { return requestState_ == RequestState::IDLE; }

  RequestState GetState() const { return requestState_; }

  void handlePacket(RR32Can::CanFrame& frame) {
    if (!isIdle()) {
      if (isFirstPacketInConfigDataStream(frame)) {
        printf("CanEngineDBStateMachine: Now downloading.\n");
        requestState_ = RequestState::DOWNLOADING;
        downloadTime = 0;
      } else if (isConfigDataStream(frame)) {
        downloadTime = 0;
      }
    }
  }

  void notifyEngineDBComplete() {
    if (!isIdle()) {
      stopRequesting();
    }
  }

  void setStatusIndicator(IStatusIndicator& si) { this->statusIndicator = &si; }

 private:
  RequestState requestState_{RequestState::IDLE};
  uint8_t tries = 0;
  uint8_t downloadTime = 0;
  CanEngineDB& engineDb_;
  IStatusIndicator* statusIndicator{nullptr};

  void stopRequesting() {
    printf("CanEngineDBStateMachine: stopRequesting()\n");
    requestState_ = RequestState::IDLE;
    stopTimer();
    if (statusIndicator != nullptr) {
      statusIndicator->clearCanDbDownload();
    }
  }

  static bool isConfigDataStream(RR32Can::CanFrame& frame) {
    return frame.id.getCommand() == RR32Can::Command::CONFIG_DATA_STREAM;
  }

  static bool isFirstPacketInConfigDataStream(RR32Can::CanFrame& frame) {
    return isConfigDataStream(frame) && (frame.data.dlc == 6 || frame.data.dlc == 7);
  }
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__CANENGINEDBSTATEMACHINE_H__
