#include "hal/LibOpencm3Hal.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <cstdio>

#include <LocoNet.h>

#include "c6021lightConfig.h"

namespace hal {

void LibOpencm3Hal::led(bool on) {
  if (on) {
    gpio_clear(kStartStopLEDBank, kStartStopLEDPin);
  } else {
    gpio_set(kStartStopLEDBank, kStartStopLEDPin);
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
  gpio_set_mode(kStartStopLEDBank, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, kStartStopLEDPin);
  gpio_set(kStartStopLEDBank, kStartStopLEDPin);  // Turn the LED off.

  gpio_set_mode(kStatusLEDBank, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, kStatusLEDPin);  // Extra LED
  gpio_set(kStatusLEDBank, kStatusLEDPin);  // Set Idle High (TODO: Correct?)

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO4 | GPIO5 | GPIO6);  // Extra LED
  gpio_set(GPIOA, GPIO4 | GPIO5 | GPIO6);
}

void LibOpencm3Hal::beginLocoNet() { LocoNet.init(PinNames::PB15); }

void LibOpencm3Hal::toggleLed() { gpio_toggle(kStartStopLEDBank, kStartStopLEDPin); }

extern "C" {
void exti14_isr();
void exti15_isr();

void exti15_10_isr() {
  if (exti_get_flag_status(EXTI14)) {
    exti14_isr();
  } else if (exti_get_flag_status(EXTI15)) {
    exti15_isr();
  }
}
}

}  // namespace hal