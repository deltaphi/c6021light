#include "tasks/RoutingTask/RoutingTask.h"

#include <atomic>
#include <cstdint>
#include <cstdio>

#include <LocoNet.h>
#include "RR32Can/RR32Can.h"
#include "hal/stm32I2C.h"
#include "hal/stm32can.h"
#include "tasks/RoutingTask/LocoNetPrinter.h"

#include "OsQueue.h"

#include "DataModel.h"

namespace tasks {
namespace RoutingTask {

namespace {
MarklinI2C::Messages::AccessoryMsg getI2CMessage(const hal::I2CBuf& buffer) {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.destination_ = DataModel::kMyAddr;
  msg.source_ = buffer.msgBytes[0];
  msg.data_ = buffer.msgBytes[1];
  return msg;
}
}  // namespace

/**
 * \brief When a message was received, create and send a response message.
 */
void RoutingTask::TaskMain() {
  while (1) {
    waitForNotify();

    // Process CAN
    constexpr const TickType_t ticksToWait = 0;
    for (hal::CanQueueType::ReceiveResult receiveResult = hal::canrxq.Receive(ticksToWait);
         receiveResult.errorCode == pdTRUE; receiveResult = hal::canrxq.Receive(ticksToWait)) {
      i2cForwarder_.forward(receiveResult.element.id, receiveResult.element.data);
      lnForwarder_.forward(receiveResult.element.id, receiveResult.element.data);
      // Forward to self
      RR32Can::RR32Can.HandlePacket(receiveResult.element.id, receiveResult.element.data);

      // Attempt to forward all updates in the CAN DB
      for (auto& locoEntry : engineDb_) {
        if (locoEntry.hasUpdate()) {
          lnForwarder_.forwardLocoChange(locoEntry.loco, locoEntry.diff);
        }
      }
    }

    // Process I2C
    for (hal::I2CQueueType::ReceiveResult receiveResult = hal::i2cRxQueue.Receive(0);
         receiveResult.errorCode == pdPASS; receiveResult = hal::i2cRxQueue.Receive(0)) {
      MarklinI2C::Messages::AccessoryMsg request = getI2CMessage(receiveResult.element);
      printf("I2C RX: ");
      request.print();

      RR32Can::Identifier rr32id;
      RR32Can::Data rr32data;

      // Convert to generic CAN representation
      if (i2cForwarder_.MakeRR32CanMsg(request, rr32id, rr32data)) {
        RR32Can::RR32Can.SendPacket(rr32id, rr32data);
        lnForwarder_.forward(rr32id, rr32data);
        // Forward to self
        RR32Can::RR32Can.HandlePacket(rr32id, rr32data);
      }
    }

    // Process LocoNet
    for (lnMsg* LnPacket = LocoNet.receive(); LnPacket; LnPacket = LocoNet.receive()) {
      printLnPacket(*LnPacket);

      RR32Can::Identifier rr32id;
      RR32Can::Data rr32data;

      // Convert to generic CAN representation
      if (lnForwarder_.MakeRR32CanMsg(*LnPacket, rr32id, rr32data)) {
        i2cForwarder_.forward(rr32id, rr32data);
        // Forward to CAN
        RR32Can::RR32Can.SendPacket(rr32id, rr32data);

        // Forward to self
        RR32Can::RR32Can.HandlePacket(rr32id, rr32data);
      }
      slotServer_.process(*LnPacket);

      // Attempt to forward all updates in the Slot server
      for (auto& locoEntry : slotServer_) {
        if (locoEntry.hasUpdate()) {
          canForwarder_.forwardLocoChange(locoEntry.loco, locoEntry.diff);
        }
      }
    }
  }
}

}  // namespace RoutingTask
}  // namespace tasks