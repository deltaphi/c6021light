#ifndef __HAL__STM32EEPROMEMULATION_H__
#define __HAL__STM32EEPROMEMULATION_H__

#include "DataModel.h"

namespace hal {

void beginEE();

void SaveConfig(const DataModel& dataModel);
DataModel LoadConfig();

}  // namespace hal

#endif  // __HAL__STM32EEPROMEMULATION_H__
