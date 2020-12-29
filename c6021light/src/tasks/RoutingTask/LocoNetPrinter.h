#ifndef __TASKS__ROUTINGTASK__LOCONETPRINTER_H__
#define __TASKS__ROUTINGTASK__LOCONETPRINTER_H__

#include <LocoNet.h>

namespace tasks {
namespace RoutingTask {

void printLnPacket(const lnMsg& LnPacket);

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETPRINTER_H__
