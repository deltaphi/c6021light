#ifndef __TASKS__ROUTINGTASK__I2CFORWARDER_H__
#define __TASKS__ROUTINGTASK__I2CFORWARDER_H__

#include "RoutingForwarder.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "hal/stm32I2C.h"

#include "DataModel.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class I2CForwarder
 */
class I2CForwarder final : public RoutingForwarder {
 public:
  void init(DataModel& dataModel) { this->dataModel_ = &dataModel; }

  void forwardLocoChange(const RR32Can::LocomotiveData& loco, LocoDiff_t& diff) override;
  void forward(const RR32Can::CanFrame& frame) override;

  bool MakeRR32CanMsg(const MarklinI2C::Messages::AccessoryMsg& i2cMsg, RR32Can::CanFrame& frame);
  bool MakeRR32CanPowerMsg(const hal::StopGoRequest& stopGoRequest, RR32Can::CanFrame& frame);

  void sendI2CResponseIfEnabled(const MarklinI2C::Messages::AccessoryMsg& i2cMsg);

 private:
  RR32Can::MachineTurnoutAddress lastPowerOnTurnoutAddr;
  RR32Can::TurnoutDirection lastPowerOnDirection;
  DataModel* dataModel_;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__I2CFORWARDER_H__
