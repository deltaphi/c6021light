#include "tasks/ConsoleTask/ConsoleTask.h"

#include <FreeRTOS.h>
#include <task.h>

#include <LocoNet.h>
#include <cstdio>

namespace tasks {
namespace ConsoleTask {

void ConsoleTask::TaskMain() {
  while (1) {
    halImpl_->loopSerial();

    // Hack: Abuse this polling task to check for LocoNet messages...
    if (LocoNet.available()) {
      // ..and wake the RoutingTask if needed.
      xTaskNotify(routingTask_, 1, eSetValueWithoutOverwrite);
    }
  }
}

}  // namespace ConsoleTask
}  // namespace tasks
