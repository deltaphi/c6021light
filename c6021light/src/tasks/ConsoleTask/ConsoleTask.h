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

  void setup(hal::LibOpencm3Hal* halImpl) { halImpl_ = halImpl; }

  void TaskMain();

 private:
  hal::LibOpencm3Hal* halImpl_;
};

}  // namespace ConsoleTask
}  // namespace tasks

#endif  // __TASKS__CONSOLETASK__CONSOLETASK_H__
