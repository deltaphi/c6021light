#ifndef __HAL__HALBASE_H__
#define __HAL__HALBASE_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/callback/TxCbk.h"

#include "ConsoleManager.h"

namespace hal {

/*
 * \brief Class HalBase
 */
class HalBase : public RR32Can::callback::TxCbk {
 public:
  void begin(uint8_t i2caddr, ConsoleManager* console) {
    i2cLocalAddr = i2caddr;
    this->console_ = console;
  }

  MarklinI2C::Messages::AccessoryMsg prepareI2cMessage();

  /**
   * \brief Send a given message over I2C.
   */
  virtual void SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) = 0;

  virtual void led(bool on) = 0;
  virtual void toggleLed() = 0;

 protected:
  static uint8_t i2cLocalAddr;
  ConsoleManager* console_;
};

}  // namespace hal

#endif  // __HAL__HALBASE_H__
