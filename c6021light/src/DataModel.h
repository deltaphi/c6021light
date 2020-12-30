#ifndef __DATAMODEL_H__
#define __DATAMODEL_H__

#include "MarklinI2C/Constants.h"
#include "RR32Can/Constants.h"
#include "tasks/RoutingTask/LocoNetSlotServer.h"

/*
 * \brief Struct DataModel
 *
 * Contains persistent configuration data.
 */
struct DataModel {
  RR32Can::RailProtocol accessoryRailProtocol = RR32Can::RailProtocol::MM2;
  constexpr static const uint8_t myAddr = MarklinI2C::kCentralAddr;
  tasks::RoutingTask::LocoNetSlotServer::SlotServerState lnSlotServerState =
      tasks::RoutingTask::LocoNetSlotServer::SlotServerState::DISABLED;
};

/*
 * \brief Struct DataAddresses
 *
 * Contains EEPROM addresses for the respective configuration data.
 */
struct DataAddresses {
  constexpr static const uint8_t startAddr = 0;
  constexpr static const uint8_t accessoryRailProtocol = startAddr + 0;
  constexpr static const uint8_t myAddr = accessoryRailProtocol + 1;
  constexpr static const uint8_t lnSlotServerState = myAddr + 1;
};

#endif  // __DATAMODEL_H__
