#ifndef __HAL__HALBASE_H__
#define __HAL__HALBASE_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/callback/TxCbk.h"

/*
 * Time functions. These match the Arduino functions, so for ARduino this is simply a duplicate
 * forward declaration.
 */

/// Give the current RTC value in seconds.
unsigned long seconds();

/// Give the current RTC value in milliseconds.
unsigned long millis();

/// Give the current RTC value in microseconds.
unsigned long micros();

namespace hal {

/*
 * \brief Class HalBase
 */
class HalBase : public RR32Can::callback::TxCbk {
 public:
  void begin(uint8_t i2caddr) { i2cLocalAddr = i2caddr; }

  MarklinI2C::Messages::AccessoryMsg prepareI2cMessage();

  /**
   * \brief Send a given message over I2C.
   */
  virtual void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) = 0;

  virtual void led(bool on) = 0;
  virtual void toggleLed() = 0;

 protected:
  static uint8_t i2cLocalAddr;
};

}  // namespace hal

#endif  // __HAL__HALBASE_H__
