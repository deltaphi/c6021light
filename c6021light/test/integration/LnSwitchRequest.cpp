#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/util/constexpr.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

using namespace ::testing;
using namespace ::mocks;
using namespace ::RR32Can::util;

namespace tasks {
namespace RoutingTask {

TEST_F(RoutingTaskFixture, Active_LnRequestSwitchState_ReturnRound) {
  dataModel.lnSlotServerState = LocoNetSlotServer::SlotServerState::ACTIVE;

  mocks::makeSequence(canHal);
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));

  const auto turnoutAddr = RR32Can::HumanTurnoutAddress(42);
  lnMsg requests = Ln_TurnoutStatusRequest(turnoutAddr);
  mocks::makeSequence(lnHal, requests);

  const lnMsg expectedPacket = Ln_LongAck(OPC_SW_STATE, true);
  EXPECT_CALL(lnTx, DoAsyncSend(expectedPacket));

  // Run!
  routingTask.loop();
}

TEST_F(RoutingTaskFixture, Passive_LnRequestSwitchState_ReturnRound) {
  dataModel.lnSlotServerState = LocoNetSlotServer::SlotServerState::PASSIVE;

  mocks::makeSequence(canHal);
  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));

  const auto turnoutAddr = RR32Can::HumanTurnoutAddress(42);
  lnMsg requests = Ln_TurnoutStatusRequest(turnoutAddr);
  mocks::makeSequence(lnHal, requests);

  // No reaction expected

  // Run!
  routingTask.loop();
}

}  // namespace RoutingTask
}  // namespace tasks