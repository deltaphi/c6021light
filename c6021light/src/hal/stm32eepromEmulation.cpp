#include "hal/stm32eepromEmulation.h"

#include <cstdio>
#include <type_traits>

#include <libopencm3/stm32/flash.h>

#include "FlashFairyPP/FlashFairyPP.h"

namespace hal {

// Extern declarations for flash pages defined in linker script
extern "C" {
  extern uint32_t _flashFairyPage0;
  extern uint32_t _flashFairyPage1;
}

FlashFairyPP::FlashFairyPP flashFairy;

void beginEE() {
  FlashFairyPP::FlashFairyPP::Config_t config;
  config.pages[0] = &_flashFairyPage0;
  config.pages[1] = &_flashFairyPage1;

  flashFairy.Init(config);
}

DataModel LoadConfig() {
  DataModel model;
  model.accessoryRailProtocol = static_cast<RR32Can::RailProtocol>(flashFairy.getValue(DataAddresses::accessoryRailProtocol));
  return model;
}

void SaveConfig(const DataModel& model) {
  uint16_t value = static_cast<std::underlying_type<RR32Can::RailProtocol>::type>(model.accessoryRailProtocol);
  flashFairy.setValue(DataAddresses::accessoryRailProtocol, value);
}

extern "C" void flash_write(uint32_t *pagePtr, uint32_t line) {
  flash_program_half_word(reinterpret_cast<uint32_t>(pagePtr) + 2, line >> 16);
  flash_program_half_word(reinterpret_cast<uint32_t>(pagePtr), line);
}

}  // namespace hal


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
