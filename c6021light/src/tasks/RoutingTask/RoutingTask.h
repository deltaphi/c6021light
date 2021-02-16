#ifndef __TASKS__ROUTINGTASK__ROUTINGTASK_H__
#define __TASKS__ROUTINGTASK__ROUTINGTASK_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"

#include "DataModel.h"

#include "OsTask.h"

#include "CANForwarder.h"
#include "I2CForwarder.h"
#include "LocoNetForwarder.h"

#include "CanEngineDB.h"
#include "LocoNetSlotServer.h"
#include "StopGoStateMachine.h"

namespace tasks {
namespace RoutingTask {

class RoutingTask : public freertossupport::OsTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void begin(DataModel& dataModel, LocoNetTx& lnTx, freertossupport::OsTimer& stopGoTimer) {
    lnForwarder_.init(dataModel, slotServer_, lnTx);
    i2cForwarder_.init(dataModel);
    slotServer_.init(dataModel, lnTx);
    stopGoStateM_.setTimer(stopGoTimer);
  };

  void TaskMain();
  void loop();

  const LocoNetSlotServer& getLnSlotServer() const { return slotServer_; }

  CanEngineDB& getCANEngineDB() { return engineDb_; }

 private:
  LocoNetSlotServer slotServer_;
  CanEngineDB engineDb_;

  CANForwarder canForwarder_;
  I2CForwarder i2cForwarder_;
  LocoNetForwarder lnForwarder_;

  void processCAN();
  void processI2CMessages();
  void processI2CStopGo();
  void processLocoNet();
  void matchEnginesFromLocoNetAndCan();

  void processStateMachines();

 public:
  StopGoStateMachine stopGoStateM_{canForwarder_, *this};
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__ROUTINGTASK_H__
