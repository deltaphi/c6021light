#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/util/constexpr.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

class CanEngineDBLoklisteDownload : public mocks::RoutingTaskFixture {};

TEST_F(CanEngineDBLoklisteDownload, Started_SendsLoklisteRequest) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal);

  EXPECT_CALL(statusIndicator, setCanDbDownload());
  EXPECT_CALL(canEngineDBTimer, Start());
  EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data("lokliste", 8)));

  routingTask.canEngineDBStateM_.startRequesting();
  routingTask.loop();
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

TEST_F(CanEngineDBLoklisteDownload, Started_ReceiveStreamHeader_StopsFurtherRequests) {
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  RR32Can::CanFrame frame = RR32Can::util::Config_Data_Stream(0, 0xAFFE);
  mocks::makeSequence(canHal, frame);

  EXPECT_CALL(statusIndicator, setCanDbDownload());
  EXPECT_CALL(canEngineDBTimer, Start());
  routingTask.canEngineDBStateM_.startRequesting();
  routingTask.loop();

  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(),
            tasks::RoutingTask::CanEngineDBStateMachine::RequestState::DOWNLOADING);
}
