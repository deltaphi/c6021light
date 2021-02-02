#ifndef __TASKS__CONSOLETASK__CONSOLETASK_H__
#define __TASKS__CONSOLETASK__CONSOLETASK_H__

#include <cstdint>

#include "OsTask.h"
#include "XpressNet/XpressNetMaster.h"

namespace tasks {
namespace ConsoleTask {

/*
 * \brief Class ConsoleTask
 */
class ConsoleTask : public freertossupport::OsTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void TaskMain();
};

}  // namespace ConsoleTask
}  // namespace tasks

#endif  // __TASKS__CONSOLETASK__CONSOLETASK_H__
