#include "hal/LibOpencm3Hal.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <cstdio>

#include "RR32Can/RR32Can.h"

#include "ee.h"
#include "eeConfig.h"

#include <LocoNet.h>

namespace hal {

void LibOpencm3Hal::led(bool on) {
  if (on) {
    gpio_clear(GPIOC, GPIO13);
  } else {
    gpio_set(GPIOC, GPIO13);
  }
}

void LibOpencm3Hal::beginClock() {
  // Enable the overall clock.
  rcc_clock_setup_in_hse_8mhz_out_72mhz();

  // Enable GPIO Pin Banks used for GPIO or alternate functions
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  // Enable Clock for alternate functions
  rcc_periph_clock_enable(RCC_AFIO);

  // Enable the UART clock
  rcc_periph_clock_enable(RCC_USART1);

  // Enable the I2C clock
  rcc_periph_clock_enable(RCC_I2C1);

  // Enable the CAN clock
  rcc_periph_clock_enable(RCC_CAN1);

  // Enable the DMA clock
  rcc_periph_clock_enable(RCC_DMA1);
}

void LibOpencm3Hal::beginGpio() {
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO13);
  gpio_set(GPIOC, GPIO13);  // Turn the LED off.

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO0);  // Extra LED
  gpio_set(GPIOA, GPIO0);  // Set Idle High (TODO: Correct?)

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO4 | GPIO5 | GPIO6);  // Extra LED
  gpio_set(GPIOA, GPIO4 | GPIO5 | GPIO6);
}

void LibOpencm3Hal::beginLocoNet() { LocoNet.init(PinNames::PB15); }

void LibOpencm3Hal::toggleLed() { gpio_toggle(GPIOC, GPIO13); }

void LibOpencm3Hal::beginEE() { ee_init(); }

DataModel LibOpencm3Hal::LoadConfig() {
  DataModel model;
  ee_read(DataAddresses::accessoryRailProtocol, sizeof(DataModel::accessoryRailProtocol),
          reinterpret_cast<uint8_t*>(&model.accessoryRailProtocol));
  return model;
}

void LibOpencm3Hal::SaveConfig(const DataModel& model) {
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