#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "tasks/RoutingTask/LocoNetSlotServer.h"

#include "mocks/RoutingTaskFixture.h"

#include "mocks/SequenceMaker.h"

class EngineRoutingFixture : public mocks::RoutingTaskFixture {
 public:
  void SetUp() {
    mocks::RoutingTaskFixture::SetUp();

    exampleLoco_.setUid(0x101010);
    exampleLoco_.setAddress(RR32Can::util::MM2_Loco(42U));
    exampleLoco_.setName("BR 42");
    exampleLoco_.setAvailability(RR32Can::LocomotiveShortInfo::AvailabilityStatus::FULL_DETAILS);

    dataModel.lnSlotServerState = tasks::RoutingTask::LocoNetSlotServer::SlotServerState::PASSIVE;

    {
      auto slotIt =
          routingTask.getLnSlotServer().findOrAllocateSlotForAddress(exampleLoco_.getAddress());
      ASSERT_NE(slotIt, routingTask.getLnSlotServer().end());
      slotIt->loco = exampleLoco_;
      slotIt->inUse = true;
      slotIt->needsMatchToCAN = true;
    }

    {
      auto engineIt = routingTask.getCANEngineDB().begin();
      engineIt->loco = exampleLoco_;
    }
  }

  RR32Can::Locomotive exampleLoco_;
  const uint8_t kLocoSlotIdx = 1;
};

namespace tasks {
namespace RoutingTask {

TEST_F(EngineRoutingFixture, SpeedChangeCanToLn) {
  RR32Can::CanFrame injectedMessage = RR32Can::util::LocoSpeed(false, exampleLoco_.getUid(), 500);
  lnMsg expectedMessage = Ln_LocoSpeed(kLocoSlotIdx, 500);

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal, injectedMessage);

  EXPECT_CALL(lnTx, DoAsyncSend(expectedMessage));

  routingTask.loop();
}

TEST_F(EngineRoutingFixture, SpeedChangeLnToCan) {
  RR32Can::CanFrame expectedMessage = RR32Can::util::LocoSpeed(false, exampleLoco_.getUid(), 496);
  lnMsg injetedMessage = Ln_LocoSpeed(kLocoSlotIdx, 500);

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal, injetedMessage);
  mocks::makeSequence(canHal);

  EXPECT_CALL(canTx, SendPacket(expectedMessage));

  routingTask.loop();
}

TEST_F(EngineRoutingFixture, DirectionChangeCanToLn) {
  RR32Can::CanFrame injectedMessage =
      RR32Can::util::LocoDirection(false, exampleLoco_.getUid(), RR32Can::EngineDirection::REVERSE);
  RR32Can::Locomotive updatedLoco = exampleLoco_;
  updatedLoco.setDirection(RR32Can::EngineDirection::REVERSE);
  ASSERT_NE(updatedLoco.getDirection(), exampleLoco_.getDirection());
  lnMsg expectedMessage = Ln_LocoDirf(kLocoSlotIdx, updatedLoco);

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal, injectedMessage);

  EXPECT_CALL(lnTx, DoAsyncSend(expectedMessage));

  routingTask.loop();
}

TEST_F(EngineRoutingFixture, DirectionChangeLnToCan) {
  RR32Can::CanFrame expectedMessage =
      RR32Can::util::LocoDirection(false, exampleLoco_.getUid(), RR32Can::EngineDirection::REVERSE);
  RR32Can::Locomotive updatedLoco = exampleLoco_;
  updatedLoco.setDirection(RR32Can::EngineDirection::REVERSE);
  ASSERT_NE(updatedLoco.getDirection(), exampleLoco_.getDirection());
  lnMsg injetedMessage = Ln_LocoDirf(kLocoSlotIdx, updatedLoco);

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal, injetedMessage);
  mocks::makeSequence(canHal);

  EXPECT_CALL(canTx, SendPacket(expectedMessage));

  routingTask.loop();
}

using FunctionFixtureParam_t = uint8_t;

class EngineRoutingFixture_F0_4 : public EngineRoutingFixture,
                                  public WithParamInterface<FunctionFixtureParam_t> {
 public:
  void SetUp() {
    EngineRoutingFixture::SetUp();

    const auto functionIdx = GetParam();

    canFrame = RR32Can::util::LocoFunction(false, exampleLoco_.getUid(), functionIdx, true);

    RR32Can::Locomotive updatedLoco = exampleLoco_;
    updatedLoco.setFunction(functionIdx, true);
    ASSERT_NE(updatedLoco.getFunction(functionIdx), exampleLoco_.getFunction(functionIdx));

    lnMsg = Ln_LocoDirf(kLocoSlotIdx, updatedLoco);
  }

  RR32Can::CanFrame canFrame;
  lnMsg lnMsg;
};

TEST_P(EngineRoutingFixture_F0_4, FunctionChangeCanToLn) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal, canFrame);

  EXPECT_CALL(lnTx, DoAsyncSend(lnMsg));

  routingTask.loop();
}

TEST_P(EngineRoutingFixture_F0_4, FunctionChangeLnToCan) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal, lnMsg);
  mocks::makeSequence(canHal);

  EXPECT_CALL(canTx, SendPacket(canFrame));

  routingTask.loop();
}

INSTANTIATE_TEST_SUITE_P(EngineRoutingFunctions, EngineRoutingFixture_F0_4,
                         Range(static_cast<FunctionFixtureParam_t>(0U),
                               static_cast<FunctionFixtureParam_t>(5U)));

}  // namespace RoutingTask
}  // namespace tasks