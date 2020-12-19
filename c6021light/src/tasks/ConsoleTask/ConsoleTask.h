#ifndef __TASKS__CONSOLETASK__CONSOLETASK_H__
#define __TASKS__CONSOLETASK__CONSOLETASK_H__

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

#include "ConsoleManager.h"

namespace tasks {
namespace ConsoleTask {

/*
 * \brief Class ConsoleTask
 */
class ConsoleTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void setup(ConsoleManager* console, TaskHandle_t routingTask) {
    console_ = console;
    routingTask_ = routingTask;
  }

  void TaskMain();

 private:
  ConsoleManager* console_;

  TaskHandle_t routingTask_;
};

}  // namespace ConsoleTask
}  // namespace tasks

#endif  // __TASKS__CONSOLETASK__CONSOLETASK_H__
