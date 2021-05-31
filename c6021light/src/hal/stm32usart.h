#ifndef __HAL__STM32USART_H__
#define __HAL__STM32USART_H__

#include <cstdint>

namespace hal {

/**
 * \brief Initialize the serial peripheral
 */
void beginSerial();

/**
 * \brief Send a string to the serial console, possibly replacing line endings.
 *
 * Searches the input string and replaces occurences of search by '\r\n'.
 *
 * \param src The string to print.
 * \param len The length of the string (strlen(src)).
 * \param doReplace Whether to replace the character denoted by search.
 * \param search The character to replace.
 *
 * \return The number of bytes read from src.
 */
std::size_t SerialWrite(const char* src, std::size_t len, bool doReplace, const char search);

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
