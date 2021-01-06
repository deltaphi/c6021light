#ifndef __HAL__STM32CAN_H__
#define __HAL__STM32CAN_H__

#include <memory>

#include "OsTask.h"

#include "RR32Can/callback/TxCbk.h"
#include "RR32Can/messages/CanFrame.h"

namespace hal {

/**
 * Pointer to an I2C message that automatically returns the memory to the RX buffer when the pointer
 * is released.
 */
using CanRxMessagePtr_t = std::unique_ptr<RR32Can::CanFrame, void (*)(RR32Can::CanFrame*)>;

void beginCan(freertossupport::OsTask taskToNotify);
CanRxMessagePtr_t getCanMessage();

class CanTxCbk : public RR32Can::callback::TxCbk {
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::CanFrame& frame) override;
};

}  // namespace hal

#endif  // __HAL__STM32CAN_H__
