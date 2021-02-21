#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "tasks/RoutingTask/LocoNetHelpers.h"
#include "tasks/RoutingTask/LocoNetSlotServer.h"

namespace tasks {
namespace RoutingTask {

TEST(SlotServer, AllocateAddress_WillReturnInitializedSlot) {
  const LocoNetSlotServer::LocoAddr_t locoAddr = RR32Can::MachineLocomotiveAddress(50U);
  LocoNetSlotServer slotServer;

  const LocoNetSlotServer::SlotDB_t::iterator it =
      slotServer.findOrAllocateSlotForAddress(locoAddr);
  EXPECT_NE(it, slotServer.end());
  EXPECT_EQ(it->loco.getAddress(), locoAddr);
}

TEST(SlotServer, extractLocoAddress) {
  const auto locoAddr = RR32Can::MachineLocomotiveAddress(50U);
  const lnMsg LnPacket = Ln_LocoAddr(locoAddr);
  EXPECT_EQ(locoAddr, LocoNetSlotServer::extractLocoAddress(LnPacket));
}

}  // namespace RoutingTask
}  // namespace tasks