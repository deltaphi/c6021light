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
  std::atomic_bool bufferOccupied;
};

using I2CQueueType = freertossupport::OsQueue<hal::I2CBuf>;

extern I2CQueueType i2cRxQueue;
extern I2CQueueType i2cTxQueue;

void beginI2C(uint8_t slaveAddress, xTaskHandle routingTaskHandle);

/**
 * Take a message from the Queue and start transmitting.
 *
 * Function from Task
 */
void startTx();

}  // namespace hal

#endif  // __HAL__STM32I2C_H__
