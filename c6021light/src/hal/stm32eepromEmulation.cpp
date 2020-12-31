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
  flashFairy.readValueIfAvailable(DataAddresses::accessoryRailProtocol,
                                  model.accessoryRailProtocol);
  flashFairy.readValueIfAvailable(DataAddresses::lnSlotServerState, model.lnSlotServerState);
  return model;
}

void SaveConfig(const DataModel& model) {
  {
    FlashFairyPP::FlashFairyPP::value_type value =
        static_cast<std::underlying_type<RR32Can::RailProtocol>::type>(model.accessoryRailProtocol);
    flashFairy.setValue(DataAddresses::accessoryRailProtocol, value);
  }
  {
    FlashFairyPP::FlashFairyPP::value_type value =
        static_cast<std::underlying_type<RR32Can::RailProtocol>::type>(model.lnSlotServerState);
    flashFairy.setValue(DataAddresses::lnSlotServerState, value);
  }
}

extern "C" void FlashFairy_Write_Word(void* pagePtr, uint32_t line) {
  flash_program_half_word(reinterpret_cast<uint32_t>(pagePtr) + 2, line >> 16);
  flash_program_half_word(reinterpret_cast<uint32_t>(pagePtr), line);
}

extern "C" void FlashFairy_Erase_Page(void* pagePtr) {
  flash_erase_page(reinterpret_cast<uint32_t>(pagePtr));
}

}  // namespace hal

namespace ConsoleManager {

int run_app_dump_flash(int, const char* const*, int) {
  FlashFairyPP::FlashFairyPP::Config_t config;
  config.pages[0] = &hal::_flashFairyPage0;
  config.pages[1] = &hal::_flashFairyPage1;

  for (unsigned int i = 0; i < 2; ++i) {
    printf("\nPage %d:\n", i);
    FlashFairyPP::FlashFairyPP::page_pointer_type page = config.pages[i];

    // 1024 bytes, hex output -> 3 columns per byte -> an 80 column line fits 26 bytes.
    // Rather display only 16 bytes to make navigation easier
    // constexpr std::size_t columnsPerByte = 3;
    constexpr std::size_t bytesPerLine = 32;
    // constexpr std::size_t indentLength = 1;
    constexpr std::size_t totalBytes = FlashFairyPP::FlashFairyPP::Config_t::pageSize;
    constexpr std::size_t totalLines = totalBytes / bytesPerLine;
    // constexpr std::size_t totalColumns = indentLength + (columnsPerByte * bytesPerLine);

    std::size_t absoluteByteIdx = 0;
    uint8_t* ptr = reinterpret_cast<uint8_t*>(page);

    for (std::size_t line = 0; line < totalLines; ++line) {
      printf(" %03x: ", absoluteByteIdx);  // Indent and base address
      for (std::size_t localByteIdx = 0; localByteIdx < bytesPerLine; ++localByteIdx) {
        printf(" %02x", *ptr);
        ++ptr;
      }
      printf("\n");
      absoluteByteIdx += bytesPerLine;
    }
  }

  return 0;
}

}  // namespace ConsoleManager