#ifndef __EECONFIG_H
#define __EECONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif


#include <stdint.h>

#define   _EE_USE_FLASH_PAGE_OR_SECTOR              (63)
#define   _EE_USE_RAM_BYTE                          (1024)
#define   _EE_VOLTAGE                               FLASH_VOLTAGE_RANGE_3 //  use in some devices

// HAL defines for libopencm3

#define FLASH_TYPEERASE_PAGES (0);
#define FLASH_TYPEPROGRAM_HALFWORD (1)


#define FLASH_BANK_1 (1);
typedef struct  {
  uint8_t NbPages;
  uint32_t PageAddress;
  uint8_t TypeErase;
  uint8_t Banks;
} FLASH_EraseInitTypeDef;

#define HAL_FLASH_Unlock() flash_unlock()
#define HAL_FLASH_Lock() flash_lock()
#define HAL_OK (0)
#define HAL_NOK (1)
#define __IO volatile

uint8_t HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef * addr, uint32_t * error);
uint8_t HAL_FLASH_Program(uint8_t flashProgramType, uint32_t addr, uint64_t data);

#ifdef __cplusplus
}
#endif

#endif
