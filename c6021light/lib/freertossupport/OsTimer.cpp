#include "OsTimer.h"
namespace freertossupport {

void StaticOsTimer::timerCallbackHelper(TimerHandle_t handle) {
  void* pvTimerID{pvTimerGetTimerID(handle)};
  if (pvTimerID != nullptr) {
    static_cast<TimerCallbackBase*>(pvTimerID)->TimerCallback(handle);
  }
}

}  // namespace freertossupport
