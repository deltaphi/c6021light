#ifndef __LOCONETTX_H__
#define __LOCONETTX_H__

#include <LocoNet.h>
#include <atomic>

/*
 * \brief Class LocoNetTx
 */
class LocoNetTx {
 public:
  LocoNetTx() : bufferFree_(true){};

  /**
   * \brief Queue a message for asynchronous transmission.
   *
   * Message may be dropped if the buffer is full.
   *
   * \return true if the message was enqueued, false otherwise.
   */
  bool AsyncSend(lnMsg& msg) {
    if (bufferFree_) {
      bufferFree_ = false;
      msgBuffer_ = msg;
      return true;
    } else {
      return false;
    }
  }

  /**
   * \brief Perform the acutal blocking send operation for a queued message.
   */
  void DoBlockingSend() {
    if (!bufferFree_) {
      LocoNet.send(&msgBuffer_);
      bufferFree_ = true;
    }
  }

 private:
  std::atomic_bool bufferFree_;
  lnMsg msgBuffer_;
};

#endif  // __LOCONETTX_H__
