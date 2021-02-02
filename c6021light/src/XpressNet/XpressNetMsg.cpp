#include "AtomicRingBuffer/ObjectRingBuffer.h"

#include "XpressNetMsg.h"

namespace XpressNetMsg {

constexpr static const uint8_t kXNQueueSize = 5;

using QueueType = AtomicRingBuffer::ObjectRingBuffer<XNetMsg, kXNQueueSize>;

QueueType XN_RxQueue;

/* TODO I honestly don't fully understand how to use the AtomicRingBuffer
 * and what the functions used here exactly do.
 */
XN_RxMsgPtr_t getXNMessage() {
  return XN_RxMsgPtr_t{XN_RxQueue.peek().ptr, freeXNRXMessage};
}

void freeXNRXMessage(XN_MsgPtr_t msgPtr) {
  XN_RxQueue.consume(QueueType::MemoryRange{msgPtr, 1});
}

void forwardRx(XNetMsg& msg) {
  // Simply put the message into the queue
  auto memory = XN_RxQueue.allocate();
  if (memory.ptr != nullptr) {
    *memory.ptr = msg;
    XN_RxQueue.publish(memory);
  }
}

}  // namespace XpressNetMsg

// TODO shall we move the notify functions to a separate file?
extern "C" void notifyXNetPower(uint8_t State) {
  XpressNetMsg::XNetMsg XN_Msg;
  XN_Msg.XN_message.header = XpressNetMsg::POWER;
  XN_Msg.XN_message.data.powerData = State;
  XpressNetMsg::forwardRx(XN_Msg);

  if (notifyXNetGlobal)
    notifyXNetGlobal();
}
