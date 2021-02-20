#include "mocks/LocoNet.h"

namespace mocks {
LocoNetClass* LocoNetInstance;
}

// Copied from LocoNet library for the purpose of stubbing
uint8_t getLnMsgSize(volatile lnMsg* Msg) {
  return ((Msg->sz.command & (uint8_t)0x60) == (uint8_t)0x60)
             ? Msg->sz.mesg_size
             : ((Msg->sz.command & (uint8_t)0x60) >> (uint8_t)4) + 2;
}

bool LocoNetTx::AsyncSend(lnMsg& msg) {
  return static_cast<mocks::LocoNetTx*>(this)->DoAsyncSend(msg);
}