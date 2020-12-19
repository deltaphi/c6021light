#ifndef __TASKS__CONSOLETASK__CONSOLETASK_H__
#define __TASKS__CONSOLETASK__CONSOLETASK_H__

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

namespace tasks {
namespace ConsoleTask {

/*
 * \brief Class ConsoleTask
 */
class ConsoleTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void setup(TaskHandle_t routingTask) { routingTask_ = routingTask; }

  void TaskMain();

 private:
  TaskHandle_t routingTask_;
};

}  // namespace ConsoleTask
}  // namespace tasks

#endif  // __TASKS__CONSOLETASK__CONSOLETASK_H__
