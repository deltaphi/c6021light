#ifndef __HAL__STM32CAN_H__
#define __HAL__STM32CAN_H__

#include "OsQueue.h"
#include "OsTask.h"

#include "RR32Can/callback/TxCbk.h"
#include "RR32Can/messages/Data.h"
#include "RR32Can/messages/Identifier.h"

namespace hal {

/*
 * \brief Class stm32can
 */
typedef struct {
  RR32Can::Identifier id;
  RR32Can::Data data;
} CanMsg;

void beginCan(freertossupport::OsTask taskToNotify);

using CanQueueType = freertossupport::OsQueue<CanMsg>;

extern CanQueueType canrxq;

class CanTxCbk : public RR32Can::callback::TxCbk {
  /// Transmit Packet on CAN
  void SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data);
};

}  // namespace hal

#endif  // __HAL__STM32CAN_H__
