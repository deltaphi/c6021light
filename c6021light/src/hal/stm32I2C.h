#ifndef __HAL__STM32I2C_H__
#define __HAL__STM32I2C_H__

#include <cstdint>
#include <memory>

#include "OsTask.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"

namespace hal {

using I2CMessage_t = MarklinI2C::Messages::AccessoryMsg;

struct OptionalI2CMessage {
  bool messageValid;
  MarklinI2C::Messages::AccessoryMsg msg;
};

void beginI2C(uint8_t slaveAddress, freertossupport::OsTask routingTask);

/**
 * Pointer to an I2C message that automatically returns the memory to the RX buffer when the pointer
 * is released.
 */
using I2CRxMessagePtr_t = std::unique_ptr<I2CMessage_t, void (*)(I2CMessage_t*)>;

/**
 * Queue a message and and start transmitting.
 *
 * Function from Task
 */
void sendI2CMessage(const I2CMessage_t& msg);

/**
 * Get the next received message.
 *
 * \return a pointer to the next message or a null pointer, if no message was available.
 */
I2CRxMessagePtr_t getI2CMessage();

}  // namespace hal

#endif  // __HAL__STM32I2C_H__
