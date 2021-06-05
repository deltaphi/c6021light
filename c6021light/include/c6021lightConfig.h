#ifndef __C6021LIGHTCONFIG_H__
#define __C6021LIGHTCONFIG_H__

#include <libopencm3/stm32/gpio.h>

constexpr static const auto kStartStopLEDBank = GPIOA;
constexpr static const auto kStartStopLEDPin = GPIO0;

constexpr static const auto kStatusLEDBank = GPIOC;
constexpr static const auto kStatusLEDPin = GPIO13;


#endif  // __C6021LIGHTCONFIG_H__
