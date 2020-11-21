#ifndef __HAL__STM32I2C_H__
#define __HAL__STM32I2C_H__

#include <atomic>
#include <cstdint>
#include "MarklinI2C/Constants.h"

#include "FreeRTOS.h"
#include "queue.h"

namespace hal {

struct I2CBuf {
  constexpr const static uint8_t kMsgBytesLength = 3;

  uint_fast8_t bytesProcessed;
  std::atomic_bool msgValid;
  uint8_t msgBytes[MarklinI2C::kMessageMaxBytes];
};

extern xQueueHandle i2cRxQueue;
extern xQueueHandle i2cTxQueue;

extern I2CBuf i2cTxBuf;

void beginI2C(uint8_t slaveAddress);
inline bool i2cTxMsgAvailable() { return i2cTxBuf.msgValid.load(std::memory_order_acquire); }

void triggerI2cTx();

}  // namespace hal

#endif  // __HAL__STM32I2C_H__
