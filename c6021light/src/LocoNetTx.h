#ifndef __LOCONETTX_H__
#define __LOCONETTX_H__

#include <LocoNet.h>

#include <AtomicRingBuffer/ObjectRingBuffer.h>

/*
 * \brief Class LocoNetTx
 */
class LocoNetTx {
 public:
  constexpr static const uint8_t QueueDepth = 8;

  /**
   * \brief Queue a message for asynchronous transmission.
   *
   * Message may be dropped if the buffer is full.
   *
   * \return true if the message was enqueued, false otherwise.
   */
  bool AsyncSend(lnMsg& msg) {
    auto memory = msgBuffer_.allocate();
    if (memory.ptr != nullptr) {
      *(memory.ptr) = msg;
      msgBuffer_.publish(memory);
      return true;
    } else {
      return false;
    }
  }

  /**
   * \brief Perform the acutal blocking send operation for a queued message.
   */
  void DoBlockingSend() {
    auto memory = msgBuffer_.peek();
    if (memory.ptr != nullptr) {
      LocoNet.send(memory.ptr);
      msgBuffer_.consume(memory);
    }
  }

 private:
  AtomicRingBuffer::ObjectRingBuffer<lnMsg, QueueDepth> msgBuffer_;
};

#endif  // __LOCONETTX_H__
