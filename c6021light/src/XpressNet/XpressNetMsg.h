#ifndef __XPRESSNETMSG_H__
#define __XPRESSNETMSG_H__

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <cstdint>
#include <memory>
#endif

#include "RR32Can/Types.h"
#include "XpressNetMaster.h"

namespace XpressNetMsg {

typedef enum {
  POWER,
  ACCESSORY_NOTIFY
} XN_Header_t;

typedef union {
  uint8_t powerData;
  uint32_t accessoryData;
} XN_MsgBody_t;

typedef struct {
  XN_Header_t header;
  XN_MsgBody_t data;
} XN_Msg_t;

/**
 * TODO I only created the class because I could not figure out
 * how to properly use the AtomicRingBuffer instead of the ObjectRingBuffer.
 * Probably this is way too convoluted and complicated for a simple message queue.
 */
class XNetMsg {
 public:

  constexpr XNetMsg(){};

  XN_Msg_t XN_message = {POWER, 0};

 private:
  
};

using XN_RxMsgPtr_t = std::unique_ptr<XNetMsg, void (*)(XNetMsg*)>;
using XN_MsgPtr_t = XNetMsg*;

XN_RxMsgPtr_t getXNMessage();
void forwardRx(XNetMsg& msg);
void freeXNRXMessage(XN_MsgPtr_t msgPtr);

}  // namespace XpressNetMsg

#endif  // __XPRESSNETMSG_H__
