#include <cstdint>
#include "FreeRTOS.h"
#include "task.h"

extern "C" {
/**
 * \brief Handler for FreeRTOS Stack Overflow detection.
 */
void vApplicationStackOverflowHook(xTaskHandle pxTask __attribute((unused)),
                                   portCHAR* pcTaskName __attribute((unused))) {
  for (;;) {
    __asm("bkpt 2");  // Loop forever here..
  }
}

/**
 * \brief Handler for Cortex-M3 Hardfaults.
 *
 * Most likely reasons to end up in here so far:
 * * Nullpointer dereference.
 * * ISR Priority not configured appropriately to be allowed to call FreeRTOS APIs.
 */
void hard_fault_handler(void) {
  uint32_t cfsr = *(uint32_t*)0xE000ED28;
  uint16_t ufsr = *(uint16_t*)0xE000ED2A;
  uint8_t bfsr = *(uint8_t*)0xE000ED29;
  uint8_t mmfsr = *(uint8_t*)0xE000ED28;

  uint32_t hfsr = *(uint32_t*)0xE000ED2C;
  __asm("bkpt 1");
  (void)cfsr;
  (void)ufsr;
  (void)bfsr;
  (void)mmfsr;
  (void)hfsr;
}
}
