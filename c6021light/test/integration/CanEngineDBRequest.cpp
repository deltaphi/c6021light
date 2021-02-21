#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/util/constexpr.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

class CanEngineDBStateMachine : public mocks::RoutingTaskFixture {};

TEST_F(CanEngineDBStateMachine, NotStarted) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  // Expect to not call any send
  EXPECT_CALL(routingTask, notify());

  // Run!
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.canEngineDBStateM_.TimerCallback(0);
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.loop();
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
}

TEST_F(CanEngineDBStateMachine, Requesting_InitialExpiry) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  EXPECT_CALL(canEngineDBTimer, Start());

  // Setup expectations
  auto frame1{RR32Can::util::Request_Config_Data(RR32Can::Filenames::kEngineNames,
                                                 RR32Can::CanDataMaxLength)};
  // frame1.id.setResponse(true);
  EXPECT_CALL(canTx, SendPacket(frame1));

  auto frame2{RR32Can::util::Request_Config_Data("0 2", 3)};
  // frame2.id.setResponse(true);
  EXPECT_CALL(canTx, SendPacket(frame2));

  // Run!
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.canEngineDBStateM_.startRequesting();
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.loop();
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
}

TEST_F(CanEngineDBStateMachine, Requesting_Expiry) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  // Setup expectations
  EXPECT_CALL(canEngineDBTimer, Start());
  EXPECT_CALL(routingTask, notify());
  EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data(RR32Can::Filenames::kEngineNames,
                                                                   RR32Can::CanDataMaxLength)));
  EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data("0 2", 3)));

  // Run!
  routingTask.canEngineDBStateM_.startRequesting();
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.canEngineDBStateM_.TimerCallback(0);
  routingTask.loop();
}

TEST_F(CanEngineDBStateMachine, Requesting_DoubleExpiry) {
  mocks::makeSequence(i2cHal, 3);
  EXPECT_CALL(i2cHal, getStopGoRequest()).Times(3).WillRepeatedly(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal, 3);
  mocks::makeSequence(canHal, 3);

  EXPECT_CALL(routingTask, notify()).Times(2);
  EXPECT_CALL(canEngineDBTimer, Start());
  EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data(RR32Can::Filenames::kEngineNames,
                                                                   RR32Can::CanDataMaxLength)))
      .Times(2);
  EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data("0 2", 3))).Times(2);

  // First Loop
  routingTask.canEngineDBStateM_.startRequesting();
  routingTask.canEngineDBStateM_.TimerCallback(0);
  routingTask.loop();

  // Second Loop
  routingTask.loop();

  // Third Loop
  routingTask.canEngineDBStateM_.TimerCallback(0);
  routingTask.loop();
}

TEST_F(CanEngineDBStateMachine, TransitionToDownloadingOnRx) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);

  EXPECT_CALL(canEngineDBTimer, Start());

  RR32Can::CanFrame canSequence[]{RR32Can::util::Config_Data_Stream(5, 0xAFFE)};
  mocks::makeSequence(canHal, canSequence);

  // Run!
  routingTask.canEngineDBStateM_.startRequesting();
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.loop();
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(),
            tasks::RoutingTask::CanEngineDBStateMachine::RequestState::DOWNLOADING);
}

TEST_F(CanEngineDBStateMachine, NoTransitionFromIdleOnRX) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);

  RR32Can::CanFrame canSequence[]{RR32Can::util::Config_Data_Stream(5, 0xAFFE)};
  mocks::makeSequence(canHal, canSequence);

  // Run!
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.loop();
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
}

TEST_F(CanEngineDBStateMachine, TransitionToIdleOnDLEnd) {
  constexpr static const uint8_t kLoopCount = 2;

  mocks::makeSequence(i2cHal, kLoopCount);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillRepeatedly(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal, kLoopCount);

  // Expect: Send Request on CAN and receive an (empty) response.

  Sequence canSequence;

  EXPECT_CALL(canEngineDBTimer, Start()).InSequence(canSequence);

  EXPECT_CALL(canHal, getCanMessage())
      .InSequence(canSequence)
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})))
      .RetiresOnSaturation();
  EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data(RR32Can::Filenames::kEngineNames,
                                                                   RR32Can::CanDataMaxLength)))
      .InSequence(canSequence);
  EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data("0 2", 3)))
      .InSequence(canSequence);

  RR32Can::CanFrame frame = RR32Can::util::Config_Data_Stream(0, 0xAFFE);
  EXPECT_CALL(canHal, getCanMessage())
      .InSequence(canSequence)
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{&frame, hal::canRxDeleter})))
      .RetiresOnSaturation();
  EXPECT_CALL(canHal, getCanMessage())
      .InSequence(canSequence)
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})))
      .RetiresOnSaturation();

  EXPECT_CALL(canEngineDBTimer, Stop()).InSequence(canSequence);

  // Run!
  EXPECT_TRUE(routingTask.getCANEngineDB().isEmpty());
  routingTask.canEngineDBStateM_.startRequesting();
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
  EXPECT_CALL(routingTask, notify());

  routingTask.canEngineDBStateM_.TimerCallback(0);
  routingTask.loop();
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());

  routingTask.loop();
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
}