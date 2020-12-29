#include "tasks/RoutingTask/LocoNetPrinter.h"

#include <cstdio>

namespace tasks {
namespace RoutingTask {

void printLnPacket(const lnMsg& LnPacket) {
  printf("LN RX: ");
  for (int i = 0; i < getLnMsgSize(const_cast<lnMsg*>(&LnPacket)); ++i) {
    printf(" %x", LnPacket.data[i]);
  }
  printf("\n");
}

}  // namespace RoutingTask
}  // namespace tasks
