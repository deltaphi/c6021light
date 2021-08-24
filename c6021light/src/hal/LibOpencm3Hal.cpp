#include "hal/LibOpencm3Hal.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include <cstdio>

#include <LocoNet.h>
#include <XpressNetMaster.h>

namespace hal {

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
  rcc_periph_clock_enable(RCC_USART2);

  // Enable the I2C clock
  rcc_periph_clock_enable(RCC_I2C1);

  // Enable the CAN clock
  rcc_periph_clock_enable(RCC_CAN1);

  // Enable the DMA clock
  rcc_periph_clock_enable(RCC_DMA1);
}

void LibOpencm3Hal::beginGpio() {
  startStopLed.init();
  statusLed.init();

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO4 | GPIO5 | GPIO6);  // Debug I/O
  gpio_set(GPIOA, GPIO4 | GPIO5 | GPIO6);

  // Tx Ena Xpressnet
  gpio_set_mode(XN_TXEN_BANK, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, XN_TXEN_PIN);
  gpio_clear(XN_TXEN_BANK, XN_TXEN_PIN);
}

void LibOpencm3Hal::beginLocoNet() { LocoNet.init(PinNames::PB15); }

void LibOpencm3Hal::beginXpressNet() { XpressNet.setup(Loco128, PinNames::PA1); }

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