#ifndef __DATAMODEL_H__
#define __DATAMODEL_H__

#include "MarklinI2C/Constants.h"
#include "RR32Can/Constants.h"

/*
 * \brief Struct DataAddresses
 *
 * Contains EEPROM addresses for the respective configuration data.
 */
struct DataAddresses {
  constexpr static const uint8_t kNumAddresses = 1;

  constexpr static const uint8_t startAddr = 0;
  constexpr static const uint8_t accessoryRailProtocol = startAddr + 0;
  constexpr static const uint8_t myAddr = accessoryRailProtocol + 1;
};

/*
 * \brief Struct DataModel
 *
 * Contains persistent configuration data.
 */
struct DataModel {
  RR32Can::RailProtocol accessoryRailProtocol = RR32Can::RailProtocol::MM2;
  constexpr static const uint8_t myAddr = MarklinI2C::kCentralAddr;

  template <typename T>
  T getValueForKey(uint8_t key) const {
    switch (key) {
      case DataAddresses::accessoryRailProtocol:
        return static_cast<T>(accessoryRailProtocol);
    }
    return 0xFF;
  }
};

#endif  // __DATAMODEL_H__
