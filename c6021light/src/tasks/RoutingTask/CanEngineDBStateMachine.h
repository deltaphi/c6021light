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
  // The request timer runs once per second. Allow five ticks without stream data.
  constexpr static const uint8_t kDownloadTimeoutTicks = 5;

  enum class RequestState { IDLE, REQUESTING, DOWNLOADING, REQUESTING_PROTOCOL };

  CanEngineDBStateMachine(CanEngineDB& engineDb, freertossupport::OsTask& parentTask)
      : StateMachineBase(parentTask), engineDb_(engineDb) {}

  void startRequesting();

  void loop();

  bool isIdle() { return requestState_ == RequestState::IDLE; }

  RequestState GetState() const { return requestState_; }

  void handlePacket(RR32Can::CanFrame& frame);

  void notifyEngineDBComplete();

  void setStatusIndicator(IStatusIndicator& si) { this->statusIndicator = &si; }

 private:
  RequestState requestState_{RequestState::IDLE};
  uint8_t tries = 0;
  uint8_t inactiveTicks_ = 0;
  CanEngineDB& engineDb_;
  IStatusIndicator* statusIndicator{nullptr};

  void startRequestingProtocol();
  void startRequestingEngineDB();
  void stopRequesting();

  bool isFirstPacketInConfigDataStream(RR32Can::CanFrame& frame);
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__CANENGINEDBSTATEMACHINE_H__
