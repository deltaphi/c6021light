#ifndef __C6021LIGHTCONFIG_H__
#define __C6021LIGHTCONFIG_H__

#include <libopencm3/stm32/gpio.h>

constexpr static const auto kStartStopLEDBank = GPIOC;
constexpr static const auto kStartStopLEDPin = GPIO13;

constexpr static const auto kStatusLEDBank = GPIOA;
constexpr static const auto kStatusLEDPin = GPIO0;


#endif  // __C6021LIGHTCONFIG_H__
