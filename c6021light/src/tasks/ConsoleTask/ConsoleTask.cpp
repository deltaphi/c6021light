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

    // Abuse polling ConsoleTask as a blocking LnTx handler
    lnTx_->DoBlockingSend();
  }
}

}  // namespace ConsoleTask
}  // namespace tasks
