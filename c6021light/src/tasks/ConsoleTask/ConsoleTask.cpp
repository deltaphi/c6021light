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
  }
}

}  // namespace ConsoleTask
}  // namespace tasks
