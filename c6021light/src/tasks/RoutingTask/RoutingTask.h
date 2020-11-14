#ifndef __TASKS__ROUTINGTASK__ROUTINGTASK_H__
#define __TASKS__ROUTINGTASK__ROUTINGTASK_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/callback/AccessoryCbk.h"

#include "DataModel.h"
#include "hal/LibOpencm3Hal.h"

#include "FreeRTOSConfig.h"

extern "C" {
void routingTaskMain(void* args);
}

namespace tasks {
namespace RoutingTask {

class RoutingTask : public RR32Can::callback::AccessoryCbk {
 public:
  static constexpr const uint32_t kStackSize = configMINIMAL_STACK_SIZE;

  void begin(DataModel& dataModel, hal::LibOpencm3Hal& halImpl) {
    this->dataModel_ = &dataModel;
    this->halImpl_ = &halImpl;
  };

  MarklinI2C::Messages::AccessoryMsg prepareI2cMessage();
  void SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg);

  void main();

  // Callback functions for RR32CanLibrary

  /**
   * \brief Called when an accessory packet was received.
   */
  void OnAccessoryPacket(RR32Can::TurnoutPacket& packet, bool response) override;

 private:
  MarklinI2C::Messages::AccessoryMsg getI2CMessage();

  uint8_t lastPowerOnTurnoutAddr;
  uint8_t lastPowerOnDirection;
  DataModel* dataModel_;
  hal::LibOpencm3Hal* halImpl_;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__ROUTINGTASK_H__
