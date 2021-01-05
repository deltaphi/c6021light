#ifndef __HAL__STM32I2C_H__
#define __HAL__STM32I2C_H__

#include <cstdint>

#include "OsTask.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"

namespace hal {

using I2CMessage_t = MarklinI2C::Messages::AccessoryMsg;
using I2CMessagePtr_t = I2CMessage_t*;


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
void sendI2CMessage(const I2CMessage_t& msg);
OptionalI2CMessage getI2CMessage();

}  // namespace hal

#endif  // __HAL__STM32I2C_H__
