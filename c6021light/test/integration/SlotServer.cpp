#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/Locomotive.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

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

using SlotServerFixture = mocks::RoutingTaskFixture;

class SlotServerActive : public SlotServerFixture {
 public:
  void SetUp() {
    SlotServerFixture::SetUp();
    this->dataModel.lnSlotServerState = LocoNetSlotServer::SlotServerState::ACTIVE;
  }

  void makeNonLnSequence() {
    mocks::makeSequence(i2cHal);
    EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
    mocks::makeSequence(canHal);
  }
};

TEST_F(SlotServerFixture, Disabled_MoveFrom0_NoReaction) {
  this->dataModel.lnSlotServerState = LocoNetSlotServer::SlotServerState::DISABLED;

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  lnMsg LnPacket = Ln_SlotMove(0, 0);

  mocks::makeSequence(lnHal, LnPacket);
  mocks::makeSequence(canHal);

  routingTask.loop();
}

TEST_F(SlotServerFixture, Passive_MoveFrom0_NoReaction) {
  this->dataModel.lnSlotServerState = LocoNetSlotServer::SlotServerState::PASSIVE;

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  lnMsg LnPacket = Ln_SlotMove(0, 0);

  mocks::makeSequence(lnHal, LnPacket);
  mocks::makeSequence(canHal);

  routingTask.loop();
}

TEST_F(SlotServerActive, MoveFrom0_NoDispatch_Nack) {
  makeNonLnSequence();

  lnMsg LnPacket = Ln_SlotMove(0, 0);
  mocks::makeSequence(lnHal, LnPacket);

  EXPECT_FALSE(routingTask.getLnSlotServer().dispatchSlotAvailable());

  lnMsg expectedPacket = Ln_LongAck(0);
  EXPECT_CALL(lnTx, DoAsyncSend(expectedPacket));

  routingTask.loop();
}

TEST_F(SlotServerActive, MoveFrom0_HasDispatch_SlotRead) {
  makeNonLnSequence();

  lnMsg LnPacket = Ln_SlotMove(0, 0);
  mocks::makeSequence(lnHal, LnPacket);

  // Expect an Engine
  const auto locoAddr = RR32Can::MachineLocomotiveAddress(50U);
  RR32Can::LocomotiveData loco{0, locoAddr, 0, RR32Can::EngineDirection::FORWARD, 0};
  lnMsg expectedPacket = Ln_SlotDataRead(1, 0x33, loco);
  EXPECT_CALL(lnTx, DoAsyncSend(expectedPacket));

  // Inject Engine into slotServer
  {
    auto slotIt = routingTask.getLnSlotServer().findOrAllocateSlotForAddress(loco.getAddress());
    ASSERT_NE(slotIt, routingTask.getLnSlotServer().end());
    slotIt->inUse = true;
    slotIt->loco = loco;
  }

  // Dispatch Engine
  routingTask.getLnSlotServer().markAddressForDispatch(locoAddr);
  EXPECT_TRUE(routingTask.getLnSlotServer().dispatchSlotAvailable());

  // Run!
  routingTask.loop();
}

TEST_F(SlotServerActive, RequestAddress_UnknownAddress_SlotRead) {
  makeNonLnSequence();

  // Expect an Engine
  const auto locoAddr = RR32Can::MachineLocomotiveAddress(50U);
  RR32Can::LocomotiveData loco{0, locoAddr, 0, RR32Can::EngineDirection::FORWARD, 0};
  lnMsg expectedPacket = Ln_SlotDataRead(1, 3, loco);
  EXPECT_CALL(lnTx, DoAsyncSend(expectedPacket));

  // Request Address
  lnMsg LnPacket = Ln_LocoAddr(locoAddr);
  mocks::makeSequence(lnHal, LnPacket);

  // Run!
  routingTask.loop();
}

TEST_F(SlotServerActive, RequestAddress_HasAddress_SlotRead) {
  makeNonLnSequence();

  // Expect an Engine
  const auto locoAddr = RR32Can::MachineLocomotiveAddress(50U);
  RR32Can::LocomotiveData loco{0, locoAddr, 15, RR32Can::EngineDirection::REVERSE, 0x001F};
  lnMsg expectedPacket = Ln_SlotDataRead(1, 0x33, loco);
  EXPECT_CALL(lnTx, DoAsyncSend(expectedPacket));

  // Inject Engine into slotServer
  {
    auto slotIt = routingTask.getLnSlotServer().findOrAllocateSlotForAddress(loco.getAddress());
    ASSERT_NE(slotIt, routingTask.getLnSlotServer().end());
    slotIt->inUse = true;
    slotIt->loco = loco;
  }

  // Request Address
  lnMsg LnPacket = Ln_LocoAddr(locoAddr);
  mocks::makeSequence(lnHal, LnPacket);

  // Run!
  routingTask.loop();
}

}  // namespace RoutingTask
}  // namespace tasks