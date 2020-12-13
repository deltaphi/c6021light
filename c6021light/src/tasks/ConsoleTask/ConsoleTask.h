#ifndef __TASKS__CONSOLETASK__CONSOLETASK_H__
#define __TASKS__CONSOLETASK__CONSOLETASK_H__

#include "hal/LibOpencm3Hal.h"

namespace tasks {
namespace ConsoleTask {

/*
 * \brief Class ConsoleTask
 */
class ConsoleTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void setup(hal::LibOpencm3Hal* halImpl, TaskHandle_t routingTask) {
    halImpl_ = halImpl;
    routingTask_ = routingTask;
  }

  void TaskMain();

 private:
  hal::LibOpencm3Hal* halImpl_;
  TaskHandle_t routingTask_;
};

}  // namespace ConsoleTask
}  // namespace tasks

#endif  // __TASKS__CONSOLETASK__CONSOLETASK_H__
