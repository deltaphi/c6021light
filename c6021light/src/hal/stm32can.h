#ifndef __HAL__STM32CAN_H__
#define __HAL__STM32CAN_H__

#include "OsTask.h"

#include "RR32Can/callback/TxCbk.h"
#include "RR32Can/messages/CanFrame.h"

namespace hal {

void beginCan(freertossupport::OsTask taskToNotify);
RR32Can::CanFrame* getCanMessage();
std::size_t freeCanMessage(RR32Can::CanFrame*);

class CanTxCbk : public RR32Can::callback::TxCbk {
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::CanFrame& frame) override;
};

}  // namespace hal

#endif  // __HAL__STM32CAN_H__
