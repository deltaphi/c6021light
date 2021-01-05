#ifndef __HAL__STM32I2C_H__
#define __HAL__STM32I2C_H__

#include <cstdint>

#include "OsTask.h"

#include "MarklinI2C/Constants.h"
#include "MarklinI2C/Messages/AccessoryMsg.h"

namespace hal {

struct I2CBuf {
  constexpr const static uint8_t kMsgBytesLength = 3;
  uint8_t msgBytes[MarklinI2C::kMessageMaxBytes];
};

struct OptionalI2CMessage {
  bool messageValid;
  MarklinI2C::Messages::AccessoryMsg msg;
};

void beginI2C(uint8_t slaveAddress, freertossupport::OsTask routingTask);

/**
 * Queue a message and and start transmitting.
 *
 * Function from Task
 */
void sendI2CMessage(const hal::I2CBuf& msg);
OptionalI2CMessage getI2CMessage();

}  // namespace hal

#endif  // __HAL__STM32I2C_H__
