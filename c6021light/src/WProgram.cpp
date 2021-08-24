#include "WProgram.h"

// Simulation of several Arduino APIs.

uint32_t digitalPinToPort(PinNames pin) {
  if (/*pin >= PinNames::PA0 &&*/ pin <= PinNames::PA15) {
    // Commented condition is always true due to limited range of data type.
    return GPIOA;
  } else if (pin >= PinNames::PB0 && pin <= PinNames::PB15) {
    return GPIOB;
  } else if (pin >= PinNames::PC0 && pin <= PinNames::PC15) {
    return GPIOC;
  } else {
    return 0;
  }
}

uint16_t digitalPinToBitNumber(PinNames pin) {
  uint8_t pinNumber = static_cast<std::underlying_type<PinNames>::type>(pin) %
                      static_cast<std::underlying_type<PinNames>::type>(PinNames::PB0);
  return pinNumber;
}

uint16_t digitalPinToBitMask(PinNames pin) {
  uint8_t pinNumber = digitalPinToBitNumber(pin);
  uint16_t result = 1 << pinNumber;
  return result;
}

void pinMode(PinNames pin, PinMode mode) {
  uint32_t port = digitalPinToPort(pin);
  uint16_t pinMask = digitalPinToBitMask(pin);

  switch (mode) {
    case PinMode::OUTPUT:
      gpio_set_mode(port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, pinMask);
      break;
    case PinMode::INPUT:
      gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, pinMask);
      break;
    case PinMode::INPUT_PULLUP:
      gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, pinMask);
      break;
  }
}

void digitalWrite(PinNames pin, PinValue value) {
  uint32_t port = digitalPinToPort(pin);
  uint16_t pinMask = digitalPinToBitMask(pin);

  switch (value) {
    case PinValue::HIGH:
      gpio_set(port, pinMask);
      break;
    case PinValue::LOW:
      gpio_clear(port, pinMask);
      break;
  }
}
