#ifndef __TASKS__ROUTINGTASK__CANFORWARDER_H__
#define __TASKS__ROUTINGTASK__CANFORWARDER_H__

#include "RoutingForwarder.h"

#include "RR32Can/RR32Can.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class CANForwarder
 */
class CANForwarder final : public RoutingForwarder {
 public:
  void forwardLocoChange(const RR32Can::LocomotiveData& loco, LocoDiff_t& diff) override;
  void forward(const RR32Can::CanFrame& frame) override {
    RR32Can::CanFrame frameToSend{frame};
    RR32Can::RR32Can.SendPacket(frameToSend);
  }
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__CANFORWARDER_H__
