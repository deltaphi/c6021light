#include "AtomicRingBuffer/ObjectRingBuffer.h"

#include "XpressNetMaster.h"
#include "XpressNetMsg.h"

namespace XpressNetMsg {

constexpr static const uint8_t kXNQueueSize = 5;

using QueueType = AtomicRingBuffer::ObjectRingBuffer<XN_Msg_t, kXNQueueSize>;

QueueType XN_RxQueue;

XN_RxMsgPtr_t getXNMessage() {
  return XN_RxMsgPtr_t{XN_RxQueue.peek().ptr, freeXNRXMessage};
}

void freeXNRXMessage(XN_MsgPtr_t msgPtr) {
  XN_RxQueue.consume(QueueType::MemoryRange{msgPtr, 1});
}

void forwardRx(XN_Msg_t& msg) {
  // Simply put the message into the queue
  auto memory = XN_RxQueue.allocate();
  if (memory.ptr != nullptr) {
    *memory.ptr = msg;
    XN_RxQueue.publish(memory);
  }
}

}  // namespace XpressNetMsg

// TODO shall we move the notify functions to a separate file?
void notifyXNetPower(uint8_t State) {
  XpressNetMsg::XN_Msg_t XN_Msg;
  XN_Msg.header = XpressNetMsg::POWER;
  XN_Msg.data.powerData = State;
  XpressNetMsg::forwardRx(XN_Msg);

  if (notifyXNetGlobal)
    notifyXNetGlobal();
}
