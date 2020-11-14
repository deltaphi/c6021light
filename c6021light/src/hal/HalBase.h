#ifndef __HAL__HALBASE_H__
#define __HAL__HALBASE_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/callback/TxCbk.h"

#include "ConsoleManager.h"

#include "DataModel.h"

namespace hal {

/*
 * \brief Class HalBase
 */
class HalBase : public RR32Can::callback::TxCbk {
 public:
  void begin(uint8_t i2caddr __attribute((unused)), ConsoleManager* console) {
    this->console_ = console;
  }

  virtual void led(bool on) = 0;
  virtual void toggleLed() = 0;

  virtual void SaveConfig(const DataModel& dataModel) = 0;
  virtual DataModel LoadConfig() = 0;

 protected:
  ConsoleManager* console_;
};

}  // namespace hal

#endif  // __HAL__HALBASE_H__
