#include "tasks/RoutingTask/CanEngineDBStateMachine.h"

#include <cstdio>

namespace tasks {
namespace RoutingTask {

void CanEngineDBStateMachine::startRequesting() {
  printf("CAN engine database download started.\n");
  requestState_ = RequestState::REQUESTING;
  if (statusIndicator != nullptr) {
    statusIndicator->setCanDbDownload();
  }
  tries = 0;
  startTimerImmediate();
}

}  // namespace RoutingTask
}  // namespace tasks
