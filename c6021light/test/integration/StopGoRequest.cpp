#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/util/constexpr.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

class StopGoStateMachine : public mocks::RoutingTaskFixture {};

TEST_F(StopGoStateMachine, NotStarted) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  // Expect to not call any send

  // Run!
  EXPECT_TRUE(routingTask.stopGoStateM_.isIdle());
  routingTask.stopGoStateM_.TimerCallback(0);
  EXPECT_TRUE(routingTask.stopGoStateM_.isIdle());
  routingTask.loop();
}

TEST_F(StopGoStateMachine, Requesting_InitialExpiry) {
  auto expectedMsg = RR32Can::util::System_GetStatus();

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  EXPECT_CALL(stopGoTimer, Start());

  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(expectedMsg));

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

  EXPECT_CALL(stopGoTimer, Start());

  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(expectedMsg));

  // Run!
  routingTask.stopGoStateM_.startRequesting();
  EXPECT_FALSE(routingTask.stopGoStateM_.isIdle());
  routingTask.stopGoStateM_.TimerCallback(0);
  routingTask.loop();
}

TEST_F(StopGoStateMachine, Requesting_DoubleExpiry) {
  auto expectedMsg = RR32Can::util::System_GetStatus();

  mocks::makeSequence(i2cHal, 3);
  EXPECT_CALL(i2cHal, getStopGoRequest()).Times(3).WillRepeatedly(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal, 3);
  mocks::makeSequence(canHal, 3);

  EXPECT_CALL(stopGoTimer, Start());

  // First Loop
  EXPECT_CALL(canTx, SendPacket(expectedMsg)).Times(2);
  routingTask.stopGoStateM_.startRequesting();
  routingTask.stopGoStateM_.TimerCallback(0);
  routingTask.loop();

  // Second Loop
  routingTask.loop();

  // Third Loop
  routingTask.stopGoStateM_.TimerCallback(0);
  routingTask.loop();
}

TEST_F(StopGoStateMachine, TransitionToIdleOnStatusRx) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);

  EXPECT_CALL(stopGoTimer, Start());

  RR32Can::CanFrame canSequence[]{RR32Can::util::System_Halt(false)};
  mocks::makeSequence(canHal, canSequence);

  EXPECT_CALL(stopGoTimer, Stop());

  // Run!
  routingTask.stopGoStateM_.startRequesting();
  EXPECT_FALSE(routingTask.stopGoStateM_.isIdle());
  routingTask.loop();
  EXPECT_TRUE(routingTask.stopGoStateM_.isIdle());
}