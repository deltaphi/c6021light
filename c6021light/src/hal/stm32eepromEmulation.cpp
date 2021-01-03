#include "hal/stm32eepromEmulation.h"

#include <cstdio>
#include <type_traits>
#include <utility>

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

  flashFairy.initialize(config);
}

DataModel LoadConfig() {
  DataModel model;
  auto loadVisitor = [&model](auto key, auto value) {
    switch (key) {
      case DataAddresses::accessoryRailProtocol:
        model.accessoryRailProtocol = static_cast<decltype(model.accessoryRailProtocol)>(value);
        break;
    }
  };

  flashFairy.visitEntries(loadVisitor);

  return model;
}

namespace {
class DataModelStoreVisitor {
 public:
  using size_type = uint32_t;
  DataModelStoreVisitor(const DataModel& model) : model_(model) {}

  class iterator {
   public:
    iterator(const DataModel& model, size_type index) : model_(model), index_(index){};

    bool operator!=(const iterator& other) const {
      return this->index_ != other.index_ || &(this->model_) != &(other.model_);
    }

    iterator& operator++() {
      ++index_;
      return *this;
    }

    auto operator*() const {
      auto result = std::make_pair(
          index_, model_.getValueForKey<FlashFairyPP::FlashFairyPP::value_type>(index_));
      return result;
    }

   private:
    const DataModel& model_;
    size_type index_;
  };

  bool contains(FlashFairyPP::FlashFairyPP::key_type key) const {
    return key < DataAddresses::kNumAddresses;
  }

  iterator begin() const { return iterator(model_, 0); }
  iterator end() const { return iterator(model_, DataAddresses::kNumAddresses); }

 private:
  const DataModel& model_;
};
}  // anonymous namespace

void SaveConfig(const DataModel& model) {
  auto storeVisitor = DataModelStoreVisitor(model);
  flashFairy.storeVisitor(storeVisitor);
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
    FlashFairyPP::FlashFairyPP::PagePtr_t page = config.pages[i];

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