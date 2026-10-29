#include "tasks/RoutingTask/CanEngineDBStateMachine.h"

#include "RR32Can/RR32Can.h"

namespace tasks {
namespace RoutingTask {

void CanEngineDBStateMachine::startRequesting() {
  timerExpired_ = false;
  RR32Can::RR32Can.AbortCurrentConfigRequest();
  engineDb_.clear();
  if (engineDb_.getDownloadProtocol() == EngineDownloadProtocol::UNKNOWN) {
    startRequestingProtocol();
  } else {
    startRequestingEngineDB();
  }
}

void CanEngineDBStateMachine::startRequestingProtocol() {
  requestState_ = RequestState::REQUESTING_PROTOCOL;
  RR32Can::RR32Can.SendPing();
  if (statusIndicator != nullptr) {
    statusIndicator->setCanDbDownload();
  }
  tries = 1;  // The initial ping has already been sent.
  startTimerDelayed();
}

void CanEngineDBStateMachine::startRequestingEngineDB() {
  requestState_ = RequestState::REQUESTING;
  if (statusIndicator != nullptr) {
    statusIndicator->setCanDbDownload();
  }
  tries = 0;
  startTimerImmediate();
}

void CanEngineDBStateMachine::loop() {
  if (requestState_ == RequestState::REQUESTING_PROTOCOL &&
      engineDb_.getDownloadProtocol() != EngineDownloadProtocol::UNKNOWN) {
    startRequestingEngineDB();
  }
  if (requestState_ == RequestState::DOWNLOADING && engineDb_.isFailed()) {
    requestState_ = RequestState::REQUESTING;
    timerExpired_ = true;
  }
  if (timerExpired_.exchange(false)) {
    if (requestState_ == RequestState::REQUESTING_PROTOCOL) {
      if (tries < kMaxDownloadTries) {
        RR32Can::RR32Can.SendPing();
        ++tries;
      } else {
        stopRequesting();
      }
    } else {
      if (requestState_ == RequestState::DOWNLOADING) {
        if (++inactiveTicks_ < kDownloadTimeoutTicks) {
          return;
        }
        requestState_ = RequestState::REQUESTING;
      }
      if (requestState_ != RequestState::REQUESTING) {
        return;
      }
      if (tries < kMaxDownloadTries) {
        RR32Can::RR32Can.AbortCurrentConfigRequest();
        engineDb_.fetchEngineDB();
        ++tries;
      } else {
        RR32Can::RR32Can.AbortCurrentConfigRequest();
        engineDb_.clear();
        stopRequesting();
      }
    }
  }
}

void CanEngineDBStateMachine::handlePacket(RR32Can::CanFrame& frame) {
  if (requestState_ == RequestState::REQUESTING && isFirstPacketInConfigDataStream(frame)) {
    requestState_ = RequestState::DOWNLOADING;
  }
  if (requestState_ == RequestState::DOWNLOADING &&
      frame.id.getCommand() == RR32Can::Command::CONFIG_DATA_STREAM) {
    inactiveTicks_ = 0;
  }
}

void CanEngineDBStateMachine::notifyEngineDBComplete() {
  if (!isIdle()) {
    stopRequesting();
  }
}

void CanEngineDBStateMachine::stopRequesting() {
  requestState_ = RequestState::IDLE;
  timerExpired_ = false;
  stopTimer();
  if (statusIndicator != nullptr) {
    statusIndicator->clearCanDbDownload();
  }
}

bool CanEngineDBStateMachine::isFirstPacketInConfigDataStream(RR32Can::CanFrame& frame) {
  return (frame.id.getCommand() == RR32Can::Command::CONFIG_DATA_STREAM) &&
         (frame.data.dlc == 6 || frame.data.dlc == 7);
}

}  // namespace RoutingTask
}  // namespace tasks
