#include "mocks/XpressNetMaster.h"

#include "XpressNet/XpressNetMsg.h"

namespace mocks {
XpressNetMasterClass* XpressNetMasterInstance;
}

void notifyXNetGlobal(void) { return; }

namespace XpressNetMsg {

XN_RxMsgPtr_t getXNMessage() { return XN_RxMsgPtr_t{nullptr, nullptr}; }

void forwardRx(XN_Msg_t& msg) { return; }

}  // namespace XpressNetMsg
