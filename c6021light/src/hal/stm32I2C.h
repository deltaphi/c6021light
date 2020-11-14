#ifndef __HAL__STM32I2C_H__
#define __HAL__STM32I2C_H__

#include <atomic>
#include <cstdint>
#include "MarklinI2C/Constants.h"

namespace hal {

struct I2CBuf {
  uint_fast8_t bytesProcessed;
  std::atomic_bool msgValid;
  uint8_t msgBytes[MarklinI2C::kMessageMaxBytes];
};

extern I2CBuf i2cRxBuf;
extern I2CBuf i2cTxBuf;

void beginI2C(uint8_t slaveAddress);
inline bool i2cRxMsgAvailable() { return i2cRxBuf.msgValid.load(std::memory_order_acquire); }
inline bool i2cTxMsgAvailable() { return i2cTxBuf.msgValid.load(std::memory_order_acquire); }

void triggerI2cTx();

}  // namespace hal

#endif  // __HAL__STM32I2C_H__
