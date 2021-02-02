#include "tasks/ConsoleTask/ConsoleTask.h"

#include "hal/stm32usart.h"

#include "ConsoleManager.h"

namespace tasks {
namespace ConsoleTask {

void ConsoleTask::TaskMain() {
  while (1) {
    uint16_t receivedCharacter = '\0';
    if (hal::pollSerial(receivedCharacter)) {
      microrl_insert_char(&ConsoleManager::microrl, receivedCharacter);
    }
    /* TODO hacky solution for first XN trials
     * It is doable to trigger a notifier whenever update() needs to be called,
     * which then triggers a task which will call update().
     */ 
    XpressNet.update();
  }
}

}  // namespace ConsoleTask
}  // namespace tasks
