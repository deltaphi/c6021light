// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

extern "C" {
#include <stdio.h>
#include <string.h>
}

#include <hal/LibOpencm3Hal.h>

using Hal_t = hal::LibOpencm3Hal;

#include "ConsoleManager.h"

#include "RR32Can/StlAdapter.h"

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

RR32Can::RailProtocol accessoryRailProtocol = RR32Can::RailProtocol::MM2;

ConsoleManager console;
// ******** Code ********

constexpr const char* MM2Name = "MM2";
constexpr const char* DCCName = "DCC";
constexpr const char* SX1Name = "SX1";

int run_app_set_turnout_protocol(int argc, const char* const* argv) {
  static constexpr const char* text{": Set Turnout protocol to "};
  if (argc < 2) {
    printf("%s: Too few arguments (1 expected).\n", argv[0]);
    return -2;
  } else if (argc > 2) {
    printf("%s: Too many arguments (1 expected).\n", argv[0]);
    return -2;
  }

  if (strncasecmp(argv[1], MM2Name, strlen(MM2Name)) == 0) {
    accessoryRailProtocol = RR32Can::RailProtocol::MM2;
    printf("%s%s'%s'.\n", argv[0], text, argv[1]);
  } else if (strncasecmp(argv[1], DCCName, strlen(DCCName)) == 0) {
    accessoryRailProtocol = RR32Can::RailProtocol::DCC;
    printf("%s%s'%s'.\n", argv[0], text, argv[1]);
  } else if (strncasecmp(argv[1], SX1Name, strlen(SX1Name)) == 0) {
    accessoryRailProtocol = RR32Can::RailProtocol::SX1;
    printf("%s%s'%s'.\n", argv[0], text, argv[1]);
  } else {
    printf("%s: Unknown rail protocol '%s'.\n", argv[0], argv[1]);
    return -3;
  }

  return 0;
}

int run_app_get_turnout_protocol(int argc, const char* const* argv) {
  if (argc > 1) {
    printf("%s: Too many arguments (0 expected).\n", argv[0]);
    return -2;
  }

  const char* turnoutProtocol = nullptr;

  switch (accessoryRailProtocol) {
    case RR32Can::RailProtocol::MM1:
    case RR32Can::RailProtocol::MM2:
    case RR32Can::RailProtocol::MFX:
      turnoutProtocol = MM2Name;
      break;
    case RR32Can::RailProtocol::DCC:
      turnoutProtocol = DCCName;
      break;
    case RR32Can::RailProtocol::SX1:
    case RR32Can::RailProtocol::SX2:
      turnoutProtocol = SX1Name;
      break;
  }

  printf("%s: The current turnout protocol is %s.\n", argv[0], turnoutProtocol);

  return 0;
}

void setup() {
  // Setup I2C & CAN
  halImpl.begin(myAddr, &console);

  console.begin();

  // Setup Serial
  printf("Connect6021Light Initializing...");

  // Tie callbacks together
  accessoryCbk.begin(halImpl);

  RR32Can::Station::CallbackStruct callbacks;
  callbacks.tx = &halImpl;
  callbacks.accessory = &accessoryCbk;
  callbacks.system = &accessoryCbk;
  RR32Can::RR32Can.begin(RR32CanUUID, callbacks);

  printf("Ready!\n");
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
          lastPowerOnTurnoutAddr, accessoryRailProtocol,
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
            lastPowerOnTurnoutAddr, accessoryRailProtocol,
            static_cast<RR32Can::TurnoutDirection>(lastPowerOnDirection), request.getPower());
      }
    }
  }

  // Process CAN: Done through callbacks.
}