#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/util/constexpr.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

class StopGoStateMachine : public mocks::RoutingTaskFixture {};

TEST_F(StopGoStateMachine, Idle) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  // Expect to not call any send

  // Run!
  EXPECT_TRUE(routingTask.stopGoStateM_.isIdle());
  routingTask.stopGoStateM_.notifyExpiry();
  EXPECT_TRUE(routingTask.stopGoStateM_.isIdle());
  routingTask.loop();
}

TEST_F(StopGoStateMachine, Requesting_NoExpiry) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  // Expect to not call any send

  // Run!
  EXPECT_TRUE(routingTask.stopGoStateM_.isIdle());
  routingTask.stopGoStateM_.startRequesting();
  EXPECT_FALSE(routingTask.stopGoStateM_.isIdle());
  routingTask.loop();
}

TEST_F(StopGoStateMachine, Requesting_Expiry) {
  auto expectedMsg = RR32Can::util::System_GetStatus();

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(expectedMsg));

  // Run!
  routingTask.stopGoStateM_.startRequesting();
  EXPECT_FALSE(routingTask.stopGoStateM_.isIdle());
  routingTask.stopGoStateM_.notifyExpiry();
  routingTask.loop();
}

TEST_F(StopGoStateMachine, Requesting_DoubleExpiry) {
  auto expectedMsg = RR32Can::util::System_GetStatus();

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  // First Loop
  EXPECT_CALL(canTx, SendPacket(expectedMsg));
  routingTask.stopGoStateM_.startRequesting();
  routingTask.stopGoStateM_.notifyExpiry();
  routingTask.loop();

  // Second Loop
  routingTask.loop();

  // Third Loop
  EXPECT_CALL(canTx, SendPacket(expectedMsg));
  routingTask.stopGoStateM_.notifyExpiry();
  routingTask.loop();
}

TEST_F(StopGoStateMachine, TransitionToIdleOnStatusRx) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);

  RR32Can::CanFrame canSequence[]{RR32Can::util::System_Halt(false)};
  mocks::makeSequence(canHal, canSequence);

  // Run!
  routingTask.stopGoStateM_.startRequesting();
  EXPECT_FALSE(routingTask.stopGoStateM_.isIdle());
  routingTask.loop();
  EXPECT_TRUE(routingTask.stopGoStateM_.isIdle());
}