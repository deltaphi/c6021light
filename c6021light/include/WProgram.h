#ifndef __WPROGRAM_H__
#define __WPROGRAM_H__

// Simulation of several Arduino APIs.
// This file is used to inject definitions into Arduino-Libraries that are now used without Arduino.

#include <cstdint>
#include <type_traits>
#include <libopencm3/stm32/gpio.h>

#include <FreeRTOS.h>
#include <portmacro.h>

/// The I/O mode of a GPIO
enum PinMode {
    OUTPUT = 0, // Assume Push-Pull
    INPUT,
    INPUT_PULLUP
};

/**
 * \brief Possible GPIO ports of the Bluepill board.
 * 
 * Note that not all of these GPIOs are physically accessible on the Bluepill. However, having them all listed
 * makes resulting computations a whole lot easier.
 */
enum PinNames: uint8_t { 
PA0 = 0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15
};

/// Macro to define ISR functions.
#define ISR(f) extern "C" void f(void)

/// Macro to obtain the contents of the output register for a GPIO*
#define portOutputRegister(port) (&(GPIO_ODR(port)))

/// Macro to obtain the contents of the input register for a GPIO*
#define portInputRegister(port) (&(GPIO_IDR(port)))
uint32_t digitalPinToPort(PinNames pin);
uint16_t digitalPinToBitMask(PinNames pin);
void pinMode(PinNames pin, PinMode mode);

/// Forwarding functions for implicit conversion
inline void pinMode(std::underlying_type<PinNames>::type pin, PinMode mode) {
    pinMode(static_cast<PinNames>(pin), mode);
}

/// Forwarding functions for implicit conversion
inline uint32_t digitalPinToPort(std::underlying_type<PinNames>::type pin) {
    return digitalPinToPort(static_cast<PinNames>(pin));
}

/// Forwarding functions for implicit conversion
inline uint16_t digitalPinToBitMask(std::underlying_type<PinNames>::type pin) {
    return digitalPinToBitMask(static_cast<PinNames>(pin));
}

#define noInterrupts() (portDISABLE_INTERRUPTS())
#define interrupts() (portENABLE_INTERRUPTS())

#define delay(ms) (vTaskDelay(pdMS_TO_TICKS(ms)))


/*
01 VBAT
02 PC 13
03 PC 14
04 PC 15
05
06
07 NRST
08
09

10 PA 0
11 PA 1
12 PA 2
13 PA 3
14 PA 4
15 PA 5
16 PA 6
17 PA 7

18 PB 0
19 PB 1
20
21 PB 10
22 PB 11
23
24
25 PB 12
26 PB 13
27 PB 14
28 PB 15

29 PA 8
30 PA 9
31 PA 10
32 PA 11
33 PA 12
34
35
36
37
38 PA 15

39 PB 3
40 PB 4
41 PB 5
42 PB 6
43 PB 7
44
45 PB 8
46 PB 9
47
48
49
*/


#endif  // __WPROGRAM_H__
