#include "tasks/ConsoleTask/ConsoleTask.h"

#include <LocoNet.h>
#include <cstdio>

namespace tasks {
namespace ConsoleTask {

void ConsoleTask::TaskMain() {
  while (1) {
    halImpl_->loopSerial();

    lnMsg * LnPacket = LocoNet.receive();
    if (LnPacket) {
      printf("LN RX: ");
      for (int i = 0; i < getLnMsgSize(LnPacket); ++i) {
        printf(" %x", LnPacket->data[i]);
      }
      printf("\n");
    }

  }
}


}  // namespace ConsoleTask
}  // namespace tasks
