#include "tasks/RoutingTask/CANForwarder.h"

namespace tasks {
namespace RoutingTask {

void CANForwarder::forwardLocoChange(const RR32Can::LocomotiveData& loco, LocoDiff_t& diff) {
  if (diff.velocity) {
    RR32Can::RR32Can.SendEngineVelocity(loco, loco.getVelocity());
    diff.velocity = false;
  }
  if (diff.direction) {
    RR32Can::RR32Can.SendEngineDirection(loco, loco.getDirection());
    diff.direction = false;
  }
  RR32Can::LocomotiveData::FunctionBits_t maskBit = 1;
  for (uint8_t functionIdx = 0; functionIdx < ((sizeof(decltype(maskBit))) * 8);
       ++functionIdx, maskBit <<= 1) {
    if ((maskBit & diff.functions) != 0) {
      RR32Can::RR32Can.SendEngineFunction(loco, functionIdx, loco.getFunction(functionIdx));
    }
  }
  diff.functions = 0;
}

}  // namespace RoutingTask
}  // namespace tasks
