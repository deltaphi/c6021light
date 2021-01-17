#ifndef __DATAMODEL_H__
#define __DATAMODEL_H__

#include "MarklinI2C/Constants.h"
#include "RR32Can/Constants.h"
#include "tasks/RoutingTask/LocoNetSlotServer.h"

/*
 * \brief Struct DataAddresses
 *
 * Contains EEPROM addresses for the respective configuration data.
 */
struct DataAddresses {
  constexpr static const uint8_t kNumAddresses = 3;

  constexpr static const uint8_t startAddr = 0;
  constexpr static const uint8_t accessoryRailProtocol = startAddr + 0;
  constexpr static const uint8_t lnSlotServerState = accessoryRailProtocol + 1;
  constexpr static const uint8_t generateI2CTurnoutResponse = lnSlotServerState + 1;
};

/*
 * \brief Struct DataModel
 *
 * Contains persistent configuration data.
 */
struct DataModel {
  RR32Can::RailProtocol accessoryRailProtocol = RR32Can::RailProtocol::MM2;
  constexpr static const uint8_t kMyAddr = MarklinI2C::kCentralAddr;
  tasks::RoutingTask::LocoNetSlotServer::SlotServerState lnSlotServerState =
      tasks::RoutingTask::LocoNetSlotServer::SlotServerState::DISABLED;
  bool generateI2CTurnoutResponse = false;

  template <typename T>
  T getValueForKey(uint8_t key) const {
    switch (key) {
      case DataAddresses::accessoryRailProtocol:
        return static_cast<T>(accessoryRailProtocol);
      case DataAddresses::lnSlotServerState:
        return static_cast<T>(lnSlotServerState);
      case DataAddresses::generateI2CTurnoutResponse:
        return generateI2CTurnoutResponse;
    }
    return 0xDEAD;
  }
};

#endif  // __DATAMODEL_H__
