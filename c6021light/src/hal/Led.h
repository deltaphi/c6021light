#ifndef __HAL__LED_H__
#define __HAL__LED_H__

#include <cstdint>

#include <libopencm3/stm32/gpio.h>

namespace hal {

/*
 * \brief Class Led
 */
template <uint32_t port, uint32_t pin>
class Led {
 public:
  void init() {
    gpio_set_mode(port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
                  pin);
    off();  // Turn the LED off.
  }

  void on() { gpio_clear(port, pin); }

  void off() { gpio_set(port, pin); }

  void onOff(bool on_param) {
    if (on_param) {
      on();
    } else {
      off();
    }
  }

  void toggle() { gpio_toggle(port, pin); }
};

}  // namespace hal

#endif  // __HAL__LED_H__
