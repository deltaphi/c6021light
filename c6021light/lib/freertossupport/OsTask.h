#ifndef __FREERTOSSUPPORT__OSTASK_H__
#define __FREERTOSSUPPORT__OSTASK_H__

#include "FreeRTOS.h"
#include "task.h"

namespace freertossupport {

/*
 * \brief Class OsTask
 */
class OsTask {
 public:
 protected:
  TaskHandle_t handle_;
};

template <typename TaskClass, uint32_t StackSize>
class StaticOsTask : public OsTask {
 public:
  void Create(TaskClass& taskImpl, const char* taskName, UBaseType_t priority) {
    handle_ = xTaskCreateStatic((void (*) (void*))  &TaskClass::TaskMain, taskName, StackSize, &taskImpl, priority,
                                stack_, &tcb_);
                                configASSERT(handle_ != NULL);
  }

 private:
  StackType_t stack_[StackSize];
  StaticTask_t tcb_;
};

}  // namespace freertossupport

#endif  // __FREERTOSSUPPORT__OSTASK_H__
