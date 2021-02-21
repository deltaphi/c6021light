#include "LocoNetTx.h"

#include <LocoNet.h>
#include "tasks/RoutingTask/LocoNetPrinter.h"

bool LocoNetTx::AsyncSend(const lnMsg& msg) {
  auto memory = msgBuffer_.allocate();
  if (memory.ptr != nullptr) {
    *(memory.ptr) = msg;
    msgBuffer_.publish(memory);
    return true;
  } else {
    return false;
  }
}

void LocoNetTx::DoBlockingSend() {
  auto memory = msgBuffer_.peek();
  if (memory.ptr != nullptr) {
    tasks::RoutingTask::printLnPacket(*memory.ptr, tasks::RoutingTask::RxTxDirection::TX);
    LocoNet.send(memory.ptr);
    msgBuffer_.consume(memory);
  }
}