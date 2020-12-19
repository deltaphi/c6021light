#ifndef __HAL__STM32USART_H__
#define __HAL__STM32USART_H__

#include <cstdint>

namespace hal {

/**
 * \brief Initialize the serial peripheral
 */
void beginSerial();

/**
 * \brief Poll the serial peripheral whether a character was received.
 *
 * \param[out] receivedChar The character that was received. Only valid if the method returns true.
 *
 * \return true if a character was received, false otherwise.
 */
bool pollSerial(uint16_t& receivedChar);

}  // namespace hal

#endif  // __HAL__STM32USART_H__
