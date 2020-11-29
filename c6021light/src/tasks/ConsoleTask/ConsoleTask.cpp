#include "tasks/ConsoleTask/ConsoleTask.h"

namespace tasks {
namespace ConsoleTask {

void ConsoleTask::TaskMain() {
  while (1) {
    halImpl_->loopSerial();
  }
}

}  // namespace ConsoleTask
}  // namespace tasks
