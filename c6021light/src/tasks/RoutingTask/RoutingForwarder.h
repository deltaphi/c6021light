#ifndef __TASKS__ROUTINGTASK__ROUTINGFORWARDER_H__
#define __TASKS__ROUTINGTASK__ROUTINGFORWARDER_H__

#include "RR32Can/Locomotive.h"
#include "RR32Can/messages/Data.h"
#include "RR32Can/messages/Identifier.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class RoutingForwarder
 */
class RoutingForwarder {
 public:
  virtual void forwardLocoChange(const RR32Can::LocomotiveData& loco, const bool velocityChange,
                                 const bool directionChange,
                                 const RR32Can::LocomotiveData::FunctionBits_t functionChanges) = 0;
  virtual void forward(const RR32Can::Identifier rr32id, const RR32Can::Data& rr32data) = 0;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__ROUTINGFORWARDER_H__
