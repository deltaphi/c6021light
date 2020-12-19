#include "hal/stm32eepromEmulation.h"

#include <cstdio>

#include "ee.h"
#include "eeConfig.h"

namespace hal {

void beginEE() { ee_init(); }

DataModel LoadConfig() {
  DataModel model;
  ee_read(DataAddresses::accessoryRailProtocol, sizeof(DataModel::accessoryRailProtocol),
          reinterpret_cast<uint8_t*>(&model.accessoryRailProtocol));
  return model;
}

void SaveConfig(const DataModel& model) {
  ee_writeToRam(DataAddresses::accessoryRailProtocol, sizeof(DataModel::accessoryRailProtocol),
                reinterpret_cast<uint8_t*>(&const_cast<DataModel&>(model).accessoryRailProtocol));
  ee_commit();
}

extern "C" uint8_t HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef* addr, uint32_t* error) {
  if (addr->NbPages == 1) {
    flash_erase_page(addr->PageAddress);
    *error = flash_get_status_flags();
    if (*error != FLASH_SR_EOP) {
      return HAL_NOK;
    } else {
      *error = 0xFFFFFFFF;  // Expected value by caller of OK case.
      return HAL_OK;
    }
  } else {
    printf("HAL_FLASHEx_Erase: requested erase not supported.\n");
    return HAL_NOK;
  }
}

extern "C" uint8_t HAL_FLASH_Program(uint8_t flashProgramType, uint32_t addr, uint64_t data) {
  if (flashProgramType == FLASH_TYPEPROGRAM_HALFWORD) {
    flash_program_half_word(addr, data);
    uint32_t flashStatus = flash_get_status_flags();
    if (flashStatus != FLASH_SR_EOP) {
      return HAL_NOK;
    } else {
      return HAL_OK;
    }
  } else {
    printf("HAL_FLASH_Program: flashProgramType not supported.\n");
    return HAL_NOK;
  }
}

}  // namespace hal
