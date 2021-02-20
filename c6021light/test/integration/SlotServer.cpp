#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/Locomotive.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

#include "tasks/RoutingTask/LocoNetSlotServer.h"

namespace tasks {
namespace RoutingTask {
using SlotServerFixture = mocks::RoutingTaskFixture;

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

TEST_F(SlotServerFixture, Active_MoveFrom0_NoDispatch_Nack) {
  this->dataModel.lnSlotServerState = LocoNetSlotServer::SlotServerState::ACTIVE;

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  lnMsg LnPacket = Ln_SlotMove(0, 0);

  mocks::makeSequence(lnHal, LnPacket);
  mocks::makeSequence(canHal);

  EXPECT_FALSE(routingTask.getLnSlotServer().dispatchSlotAvailable());

  lnMsg expectedPacket = Ln_LongAck(0);
  EXPECT_CALL(lnTx, DoAsyncSend(expectedPacket));

  routingTask.loop();
}

TEST_F(SlotServerFixture, Active_MoveFrom0_HasDispatch_SlotRead) {
  this->dataModel.lnSlotServerState = LocoNetSlotServer::SlotServerState::ACTIVE;

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  lnMsg LnPacket = Ln_SlotMove(0, 0);

  mocks::makeSequence(lnHal, LnPacket);
  mocks::makeSequence(canHal);

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

}  // namespace RoutingTask
}  // namespace tasks