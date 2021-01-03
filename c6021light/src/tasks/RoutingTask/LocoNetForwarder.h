#ifndef __TASKS__ROUTINGTASK__LOCONETFORWARDER_H__
#define __TASKS__ROUTINGTASK__LOCONETFORWARDER_H__

#include "RoutingForwarder.h"

#include "RR32Can/Locomotive.h"
#include "RR32Can/messages/Data.h"
#include "RR32Can/messages/Identifier.h"

#include "ln_opc.h"

#include "DataModel.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class LocoNetForwarder
 */
class LocoNetForwarder final : public RoutingForwarder {
 public:
  void init(DataModel& dataModel) { this->dataModel_ = &dataModel; }

  void forwardLocoChange(const RR32Can::LocomotiveData& loco, const bool velocityChange,
                         const bool directionChange,
                         const RR32Can::LocomotiveData::FunctionBits_t functionChanges) override;
  void forward(const RR32Can::Identifier rr32id, const RR32Can::Data& rr32data) override;

  bool MakeRR32CanMsg(const lnMsg& LnPacket, RR32Can::Identifier& rr32id, RR32Can::Data& rr32data);

 private:
  DataModel* dataModel_;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETFORWARDER_H__
