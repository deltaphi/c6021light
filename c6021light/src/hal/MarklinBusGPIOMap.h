#ifndef __HAL__MARKLINBUSGPIOMAP_H__
#define __HAL__MARKLINBUSGPIOMAP_H__

#include <libopencm3/stm32/gpio.h>

namespace hal {
namespace MarklinBusGPIOMap {

struct GPIODef {
  uint32_t port;
  uint16_t bits;
};

constexpr static const GPIODef SDA_SCL{GPIOB, GPIO_I2C1_SCL | GPIO_I2C1_SDA};

constexpr static const GPIODef STOP{GPIOB, GPIO4};
constexpr static const GPIODef GO{GPIOA, GPIO15};

constexpr static const GPIODef INIT{GPIOB, GPIO3};

constexpr static const GPIODef B10{GPIOA, GPIO8};

}  // namespace MarklinBusGPIOMap
}  // namespace hal

#endif  // __HAL__MARKLINBUSGPIOMAP_H__
