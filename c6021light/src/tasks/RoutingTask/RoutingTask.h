#ifndef __TASKS__ROUTINGTASK__ROUTINGTASK_H__
#define __TASKS__ROUTINGTASK__ROUTINGTASK_H__

#include "MarklinI2C/Messages/AccessoryMsg.h"

#include "DataModel.h"
#include "hal/stm32I2C.h"
#include "hal/stm32can.h"

#include "FreeRTOSConfig.h"

#include "LocoNet.h"

#include "LocoNetSlotServer.h"

namespace tasks {
namespace RoutingTask {

class RoutingTask {
 public:
  static constexpr const uint32_t kStackSize = 256;

  void begin(DataModel& dataModel) {
    this->dataModel_ = &dataModel;
    slotServer_.init(dataModel);
  };

  void TaskMain();

  hal::CanTxCbk canTxCbk_;

  const LocoNetSlotServer& getLnSlotServer() const { return slotServer_; }

 private:
  MarklinI2C::Messages::AccessoryMsg prepareI2cMessage();
  void SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg);
  MarklinI2C::Messages::AccessoryMsg getI2CMessage(hal::I2CBuf& buffer);

  bool MakeRR32CanMsg(const lnMsg& LnPacket, RR32Can::Identifier& rr32id, RR32Can::Data& rr32data);
  bool MakeRR32CanMsg(const MarklinI2C::Messages::AccessoryMsg& i2cMsg, RR32Can::Identifier& rr32id,
                      RR32Can::Data& rr32data);
  void ForwardToLoconet(const RR32Can::Identifier rr32id, const RR32Can::Data& rr32data);
  void ForwardToI2C(const RR32Can::Identifier rr32id, const RR32Can::Data& rr32data);

  RR32Can::MachineTurnoutAddress lastPowerOnTurnoutAddr;
  RR32Can::TurnoutDirection lastPowerOnDirection;
  DataModel* dataModel_;
  LocoNetSlotServer slotServer_;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__ROUTINGTASK_H__
