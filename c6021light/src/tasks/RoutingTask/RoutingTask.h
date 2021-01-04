#ifndef __TASKS__ROUTINGTASK__ROUTINGTASK_H__
#define __TASKS__ROUTINGTASK__ROUTINGTASK_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"

#include "DataModel.h"
#include "hal/stm32can.h"

#include "FreeRTOSConfig.h"
#include "OsTask.h"

#include "LocoNet.h"

#include "CANForwarder.h"
#include "I2CForwarder.h"
#include "LocoNetForwarder.h"

#include "CanEngineDB.h"
#include "LocoNetSlotServer.h"

namespace tasks {
namespace RoutingTask {

class RoutingTask : public freertossupport::OsTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void begin(DataModel& dataModel) {
    lnForwarder_.init(dataModel, slotServer_);
    i2cForwarder_.init(dataModel);
    slotServer_.init(dataModel);
  };

  void TaskMain();

  hal::CanTxCbk canTxCbk_;

  const LocoNetSlotServer& getLnSlotServer() const { return slotServer_; }

  CanEngineDB& getCANEngineDB() { return engineDb_; }

 private:
  LocoNetSlotServer slotServer_;
  CanEngineDB engineDb_;

  CANForwarder canForwarder_;
  I2CForwarder i2cForwarder_;
  LocoNetForwarder lnForwarder_;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__ROUTINGTASK_H__
