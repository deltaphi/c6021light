#ifndef __TASKS__ROUTINGTASK__ROUTINGFORWARDER_H__
#define __TASKS__ROUTINGTASK__ROUTINGFORWARDER_H__

#include "RR32Can/Locomotive.h"
#include "RR32Can/messages/CanFrame.h"

namespace tasks {
namespace RoutingTask {

struct LocoDiff_t {
  bool velocity = false;
  bool direction = false;
  RR32Can::Locomotive::FunctionBits_t functions = 0;

  bool hasDiff() const { return velocity || direction || (functions != 0); }
};

/*
 * \brief Class RoutingForwarder
 */
class RoutingForwarder {
 public:
  /**
   * Forward a message for stateful forwarding (Engine control).
   *
   * When the information has been forwarded, the bits in diff will be reset.
   * Note that the egress bus might have to create some state, first. The message will then not be
   * forwarded and diff will remain untouched, so that the information can be retransmitted later.
   */
  virtual void forwardLocoChange(const RR32Can::LocomotiveData& loco, LocoDiff_t& diff) = 0;

  /**
   * Forward a message for stateless forwarding (Power On/Off, Turnout Request/Response, S88 Report)
   */
  virtual void forward(const RR32Can::CanFrame& frame) = 0;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__ROUTINGFORWARDER_H__
