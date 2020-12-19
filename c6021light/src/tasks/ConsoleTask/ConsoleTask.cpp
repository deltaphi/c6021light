#include "tasks/ConsoleTask/ConsoleTask.h"

#include <FreeRTOS.h>
#include <task.h>

#include <LocoNet.h>
#include <cstdio>

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

    // Hack: Abuse this polling task to check for LocoNet messages...
    if (LocoNet.available()) {
      // ..and wake the RoutingTask if needed.
      xTaskNotify(routingTask_, 1, eSetValueWithoutOverwrite);
    }
  }
}

}  // namespace ConsoleTask
}  // namespace tasks
