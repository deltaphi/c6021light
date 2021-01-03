#include "tasks/RoutingTask/CANForwarder.h"

namespace tasks {
namespace RoutingTask {

void CANForwarder::forwardLocoChange(
    const RR32Can::LocomotiveData& loco, const bool velocityChange, const bool directionChange,
    const RR32Can::LocomotiveData::FunctionBits_t functionChanges) {
  if (velocityChange) {
    RR32Can::RR32Can.SendEngineVelocity(loco, loco.getVelocity());
  }
  if (directionChange) {
    RR32Can::RR32Can.SendEngineDirection(loco, loco.getDirection());
  }
  RR32Can::LocomotiveData::FunctionBits_t maskBit = 1;
  for (uint8_t functionIdx = 0; functionIdx < ((sizeof(decltype(maskBit))) * 8);
       ++functionIdx, maskBit <<= 1) {
    if ((maskBit & functionChanges) != 0) {
      RR32Can::RR32Can.SendEngineFunction(loco, functionIdx, loco.getFunction(functionIdx));
    }
  }
}

}  // namespace RoutingTask
}  // namespace tasks
