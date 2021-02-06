#ifndef __TASKS__CONSOLETASK__CONSOLETASK_H__
#define __TASKS__CONSOLETASK__CONSOLETASK_H__

#include <cstdint>

#include "OsTask.h"

#include "LocoNetTx.h"

namespace tasks {
namespace ConsoleTask {

/*
 * \brief Class ConsoleTask
 */
class ConsoleTask : public freertossupport::OsTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void setup(LocoNetTx& lnTx) { lnTx_ = &lnTx; }

  void TaskMain();

 private:
  LocoNetTx* lnTx_;
};

}  // namespace ConsoleTask
}  // namespace tasks

#endif  // __TASKS__CONSOLETASK__CONSOLETASK_H__
