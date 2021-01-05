#ifndef __TASKS__ROUTINGTASK__I2CFORWARDER_H__
#define __TASKS__ROUTINGTASK__I2CFORWARDER_H__

#include "RoutingForwarder.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"

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

 private:
  RR32Can::MachineTurnoutAddress lastPowerOnTurnoutAddr;
  RR32Can::TurnoutDirection lastPowerOnDirection;
  DataModel* dataModel_;

  static MarklinI2C::Messages::AccessoryMsg prepareI2cMessage();
  void SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg);
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__I2CFORWARDER_H__
