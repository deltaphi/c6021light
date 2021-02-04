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


using XN_RxMsgPtr_t = std::unique_ptr<XN_Msg_t, void (*)(XN_Msg_t*)>;
using XN_MsgPtr_t = XN_Msg_t*;

XN_RxMsgPtr_t getXNMessage();
void forwardRx(XN_Msg_t& msg);
void freeXNRXMessage(XN_MsgPtr_t msgPtr);

}  // namespace XpressNetMsg

#endif  // __XPRESSNETMSG_H__
