#ifndef __FREERTOSSUPPORT__OSTASK_H__
#define __FREERTOSSUPPORT__OSTASK_H__

#include <type_traits>

#include "FreeRTOS.h"
#include "task.h"

namespace freertossupport {

/*
 * \brief Class OsTask
 */
class OsTask {
 public:
  TaskHandle_t getHandle() { return handle_; }

  void waitForNotify() { ulTaskNotifyTake(pdTRUE, portMAX_DELAY); }

  void notify() { xTaskNotify(handle_, 1, eSetValueWithoutOverwrite); }

  BaseType_t notifyFromISR() {
    BaseType_t HigherPriorityTaskWoken;
    xTaskNotifyFromISR(handle_, 1, eSetValueWithoutOverwrite, &HigherPriorityTaskWoken);
    return HigherPriorityTaskWoken;
  }

  void notifyFromISRWithWake() {
    bool higherPriorityTaskWoken = notifyFromISR() == pdTRUE;
    if (higherPriorityTaskWoken) {
      taskYIELD();
    }
  }

 protected:
  TaskHandle_t handle_;
};

template <typename TaskClass, uint32_t StackSize>
class StaticOsTask : public TaskClass {
 public:
  static_assert(std::is_base_of<OsTask, TaskClass>::value,
                "TaskClass must extend freertossupport::OsTask.");

  void Create(const char* taskName, UBaseType_t priority) {
    this->handle_ = xTaskCreateStatic((void (*)(void*))&TaskClass::TaskMain, taskName, StackSize,
                                      this, priority, stack_, &tcb_);
    configASSERT(this->handle_ != NULL);
  }

 private:
  StackType_t stack_[StackSize];
  StaticTask_t tcb_;
};

}  // namespace freertossupport

#endif  // __FREERTOSSUPPORT__OSTASK_H__
