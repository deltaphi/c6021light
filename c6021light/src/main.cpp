// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

#ifdef ARDUINO
#include <hal/ArduinoUnoHal.h>
using Hal_t = hal::ArduinoUnoHal;
#elif defined(PLATFORMIO_FRAMEWORK_libopencm3)
#include <hal/LibOpencm3Hal.h>
using Hal_t = hal::LibOpencm3Hal;
#endif

extern "C" {
#include "microrl.h"
}

#include "RR32Can/StlAdapter.h"
#include "hal/PrintfAb.h"

#include "StationCbk.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"

#include "RR32Can/RR32Can.h"
#include "RR32Can_config.h"

// ******** Variables and Constans********
Hal_t halImpl;
AccessoryCbk accessoryCbk;

uint8_t lastPowerOnTurnoutAddr;
uint8_t lastPowerOnDirection;

constexpr const uint8_t myAddr = MarklinI2C::kCentralAddr;

constexpr const RR32Can::RailProtocol kAccessoryRailProtocol = RR32Can::RailProtocol::MM2;

microrl_t microrl;

// ******** Code ********

void microrl_print_cbk(const char* s) {
  printf(s);
  fflush(stdout);
}

int microrl_execute_callback(int argc, const char* const* argv) {
  printf("microrl_execute: %i args\n", argc);
  return 0;
}

void setup() {

  microrl_init(&microrl, microrl_print_cbk);

  // Setup I2C & CAN
  halImpl.begin(myAddr, &microrl);

  // Setup Serial
  MYPRINTF("Connect6021Light Initializing...");
  
  // Tie callbacks together
  accessoryCbk.begin(halImpl);

  RR32Can::Station::CallbackStruct callbacks;
  callbacks.tx = &halImpl;
  callbacks.accessory = &accessoryCbk;
  callbacks.system = &accessoryCbk;
  RR32Can::RR32Can.begin(RR32CanUUID, callbacks);

  microrl_set_execute_callback(&microrl, microrl_execute_callback);

  MYPRINTF("Ready!\n");
}

/**
 * \brief Two addresses are on the same decoder, if they match apart from
 * the lowest two bits.
 */
bool sameDecoder(uint8_t left, uint8_t right) {
  constexpr const uint8_t mask = 0xFC;
  return (left & mask) == (right & mask);
}

/**
 * \brief When a message was received, create and send a response message.
 */
void loop() {
  halImpl.loop();

  // Process I2C
  if (halImpl.i2cAvailable()) {
    MarklinI2C::Messages::AccessoryMsg request = halImpl.getI2cMessage();

    // If this is a power ON packet: Send directly to CAN
    if (request.getPower()) {
      lastPowerOnDirection = request.getDirection();
      lastPowerOnTurnoutAddr = request.getTurnoutAddr();
      RR32Can::RR32Can.SendAccessoryPacket(
          lastPowerOnTurnoutAddr, kAccessoryRailProtocol,
          static_cast<RR32Can::TurnoutDirection>(request.getDirection()), request.getPower());
    } else {
      // On I2C, for a Power OFF message, the two lowest bits (decoder output channel) are always 0,
      // regardless of the actual turnout address to be switched off. For safety, translate this to
      // tuning off all addresses of the respective decoder.
      //
      // Note that we store the last direction where power was applied.
      // The CAN side interprets a "Power Off" as "Flip the switch" anyways.
      uint8_t i2cAddr = request.getTurnoutAddr();
      if (sameDecoder(i2cAddr, lastPowerOnTurnoutAddr)) {
        RR32Can::RR32Can.SendAccessoryPacket(
            lastPowerOnTurnoutAddr, kAccessoryRailProtocol,
            static_cast<RR32Can::TurnoutDirection>(lastPowerOnDirection), request.getPower());
      }
    }
  }

  // Process CAN: Done through callbacks.
}