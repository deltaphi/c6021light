#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mocks/LocoNet.h"
#include "mocks/RoutingForwarderMock.h"

#include "RR32Can/RR32Can.h"

#include "tasks/RoutingTask/LocoNetHelpers.h"
#include "tasks/RoutingTask/RoutingTask.h"

#include "RR32Can/messages/S88Event.h"
#include "RR32Can/util/constexpr.h"

#include "mocks/SequenceMaker.h"

using namespace ::testing;
using namespace ::RR32Can::util;

namespace tasks {
namespace RoutingTask {

class StatelessRoutingFixture : public Test {
 public:
  void SetUp() {
    mocks::LocoNetInstance = &lnHal;
    hal::canMock = &canHal;
    hal::i2cMock = &i2cHal;

    routingTask.begin(dataModel);

    RR32Can::Station::CallbackStruct callbacks;
    callbacks.tx = &canTx;
    RR32Can::RR32Can.begin(0, callbacks);
  }

  void TearDown() {
    hal::canMock = nullptr;
    hal::i2cMock = nullptr;
    mocks::LocoNetInstance = nullptr;
  }

  DataModel dataModel;
  StrictMock<hal::I2CHalMock> i2cHal;
  StrictMock<hal::CANHalMock> canHal;
  StrictMock<hal::CanTxMock> canTx;
  mocks::LocoNetClass lnHal;
  RoutingTask routingTask;
};

using TurnoutTestParam_t = RR32Can::MachineTurnoutAddress;

class TurnoutRoutingFixture : public StatelessRoutingFixture,
                              public WithParamInterface<TurnoutTestParam_t> {
 public:
  constexpr static const RR32Can::TurnoutDirection direction = RR32Can::TurnoutDirection::GREEN;
  constexpr static const bool power = true;

  RR32Can::MachineTurnoutAddress turnout{GetParam()};
  RR32Can::CanFrame canFrame{Turnout(false, turnout, direction, power)};
  hal::I2CMessage_t i2cMessage{
      MarklinI2C::Messages::AccessoryMsg::makeInbound(turnout, direction, power)};
  lnMsg LnPacket{Ln_Turnout(turnout, direction, power)};
};

TEST_P(TurnoutRoutingFixture, TurnoutRequest_I2CtoCANandLocoNet) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));
  EXPECT_CALL(lnHal, send(Pointee(LnPacket)));

  mocks::makeSequence(canHal);
  EXPECT_CALL(lnHal, receive).WillOnce(Return(nullptr));

  // Inject I2C message
  hal::I2CMessage_t i2cMessages[] = {i2cMessage};
  mocks::makeSequence(i2cHal, i2cMessages);
  
  // Run!
  routingTask.loop();
}

TEST_P(TurnoutRoutingFixture, TurnoutRequest_CANtoLocoNetAndI2C) {
  // Setup expectations
  EXPECT_CALL(lnHal, send(Pointee(LnPacket)));

  mocks::makeSequence(i2cHal);

  EXPECT_CALL(lnHal, receive).WillOnce(Return(nullptr));

  // Inject CAN message
  RR32Can::CanFrame canFrames[] = {canFrame};
  mocks::makeSequence(canHal, canFrames);

  // Run!
  routingTask.loop();
}

TEST_P(TurnoutRoutingFixture, TurnoutRequest_LocoNetToCANandI2C) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));

  EXPECT_CALL(i2cHal, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));
  mocks::makeSequence(canHal);

  // Inject LocoNet message
  EXPECT_CALL(lnHal, receive).WillOnce(Return(&LnPacket)).WillOnce(Return(nullptr));

  // Run!
  routingTask.loop();
}

INSTANTIATE_TEST_SUITE_P(TurnoutTest, TurnoutRoutingFixture,
                         Values(MM2_Turnout(0u), MM2_Turnout(1u), MM2_Turnout(5u), MM2_Turnout(10u),
                                MM2_Turnout(42u), MM2_Turnout(100u), MM2_Turnout(255u)));
  // Setup expectations
  EXPECT_CALL(lnHal, send(Pointee(LnPacket)));

  EXPECT_CALL(i2cHal, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));

  EXPECT_CALL(lnHal, receive).WillOnce(Return(nullptr));

  // Inject CAN message
  RR32Can::CanFrame canFrames[] = {canFrame};
  mocks::makeSequence(canHal, canFrames);

  // Run!
  routingTask.loop();
}

TEST_P(TurnoutRoutingFixture, TurnoutRequest_LocoNetToCAN) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));

  EXPECT_CALL(i2cHal, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));
  mocks::makeSequence(canHal);

  // Inject LocoNet message
  EXPECT_CALL(lnHal, receive).WillOnce(Return(&LnPacket)).WillOnce(Return(nullptr));

  // Run!
  routingTask.loop();
}

using namespace RR32Can;

INSTANTIATE_TEST_SUITE_P(TurnoutTest, TurnoutRoutingFixture,
                         Values(MM2_Turnout(0u), MM2_Turnout(1u), MM2_Turnout(5u), MM2_Turnout(10u),
                                MM2_Turnout(42u), MM2_Turnout(100u), MM2_Turnout(255u)));

}  // namespace RoutingTask
}  // namespace tasks