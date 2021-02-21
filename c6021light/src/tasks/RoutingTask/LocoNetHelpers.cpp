#include "tasks/RoutingTask/LocoNetHelpers.h"

namespace tasks {
namespace RoutingTask {

void dirfToLoco(const uint8_t dirf, RR32Can::LocomotiveData& loco) {
  RR32Can::EngineDirection direction;
  if ((dirf & kDirfDirMask) == kDirfDirMask) {
    direction = RR32Can::EngineDirection::REVERSE;
  } else {
    direction = RR32Can::EngineDirection::FORWARD;
  }
  loco.setDirection(direction);

  uint8_t functionMask = 1;
  for (uint8_t i = 0; i < kFunctionsInDirfMessage; ++i) {
    uint8_t functionIdx = i + 1;
    if (functionIdx == kFunctionsInDirfMessage) {
      functionIdx = 0;
    }
    loco.setFunction(functionIdx, ((dirf & functionMask) != 0));
    functionMask <<= 1;
  }
}

uint8_t locoToDirf(const RR32Can::LocomotiveData& loco) {
  uint8_t dirf = 0;

  if (loco.getDirection() == RR32Can::EngineDirection::REVERSE) {
    dirf |= kDirfDirMask;
  } else {
    dirf &= ~kDirfDirMask;
  }

  uint8_t functionMask = 1;
  for (uint8_t i = 0; i < kFunctionsInDirfMessage; ++i) {
    uint8_t functionIdx = i + 1;
    if (functionIdx == kFunctionsInDirfMessage) {
      functionIdx = 0;
    }
    if (loco.getFunction(functionIdx)) {
      dirf |= functionMask;
    } else {
      dirf &= ~functionMask;
    }
    functionMask <<= 1;
  }

  return dirf;
}

void sndToLoco(const uint8_t snd, RR32Can::LocomotiveData& loco) {
  uint8_t functionMask = 1;
  for (uint8_t functionIdx = kFunctionsInSndMessage;
       functionIdx < kLowestFunctionInSndMessage + kFunctionsInSndMessage; ++functionIdx) {
    loco.setFunction(functionIdx, ((snd & functionMask) != 0));
    functionMask <<= 1;
  }
}

uint8_t locoToSnd(const RR32Can::LocomotiveData& loco) {
  uint8_t snd = 0;

  uint8_t functionMask = 1;
  for (uint8_t i = kFunctionsInSndMessage; i < kLowestFunctionInSndMessage + kFunctionsInSndMessage;
       ++i) {
    uint8_t functionIdx = i;
    if (loco.getFunction(functionIdx)) {
      snd |= functionMask;
    } else {
      snd &= ~functionMask;
    }
    functionMask <<= 1;
  }
  return snd;
}
}  // namespace RoutingTask
}  // namespace tasks
