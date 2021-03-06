#ifndef __TASKS__ROUTINGTASK__LOCONETFORWARDER_H__
#define __TASKS__ROUTINGTASK__LOCONETFORWARDER_H__

#include "RoutingForwarder.h"

#include "RR32Can/Locomotive.h"
#include "RR32Can/messages/Data.h"
#include "RR32Can/messages/Identifier.h"

#include "ln_opc.h"

#include "DataModel.h"
#include "LocoNetTx.h"
#include "tasks/RoutingTask/LocoNetSlotServer.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class LocoNetForwarder
 */
class LocoNetForwarder final : public RoutingForwarder {
 public:
  void init(DataModel& dataModel, LocoNetSlotServer& slotServer, LocoNetTx& tx) {
    this->dataModel_ = &dataModel;
    this->slotServer_ = &slotServer;
    this->tx_ = &tx;
  }

  void forwardLocoChange(const RR32Can::LocomotiveData& loco, LocoDiff_t& diff) override;
  void forward(const RR32Can::CanFrame& frame) override;

  bool MakeRR32CanMsg(const lnMsg& LnPacket, RR32Can::CanFrame& frame);

  void HandleDummyMessages(const lnMsg& msg);

 private:
  DataModel* dataModel_ = nullptr;
  LocoNetSlotServer* slotServer_ = nullptr;
  LocoNetTx* tx_ = nullptr;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETFORWARDER_H__
