#ifndef __HAL__STM32I2C_H__
#define __HAL__STM32I2C_H__

#include <atomic>
#include <cstdint>

#include "OsQueue.h"

#include "MarklinI2C/Constants.h"

namespace hal {

struct I2CBuf {
  constexpr const static uint8_t kMsgBytesLength = 3;

  uint8_t msgBytes[MarklinI2C::kMessageMaxBytes];
};

struct I2CTxBuf : public I2CBuf {
  uint_fast8_t bytesProcessed;
  std::atomic_bool msgValid;
};

using I2CQueueType = freertossupport::OsQueue<hal::I2CBuf>;

extern I2CQueueType i2cRxQueue;
extern I2CQueueType i2cTxQueue;

void beginI2C(uint8_t slaveAddress);

void triggerI2cTx();

}  // namespace hal

#endif  // __HAL__STM32I2C_H__
