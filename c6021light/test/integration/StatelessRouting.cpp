#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mocks/KeyboardMock.h"
#include "mocks/LocoNet.h"
#include "mocks/RoutingForwarderMock.h"

#include "RR32Can/RR32Can.h"

#include "tasks/RoutingTask/RoutingTask.h"

using namespace ::testing;

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

using TurnoutTestParam_t = uint32_t;

class TurnoutRoutingFixture : public StatelessRoutingFixture,
                              public WithParamInterface<TurnoutTestParam_t> {
 public:
  void SetUp() {
    StatelessRoutingFixture::SetUp();
    { turnout.setProtocol(RR32Can::RailProtocol::MM2); }
    {
      canFrame.id.setCommand(RR32Can::Command::ACCESSORY_SWITCH);
      RR32Can::TurnoutPacket pkt(canFrame.data);
      pkt.initData();
      pkt.setDirection(direction);
      pkt.setPower(power);
      pkt.setLocid(turnout);
    }
    { i2cMessage = mocks::makeReceivedAccessoryMsg(turnout, direction, power); }
  }

  constexpr static const RR32Can::TurnoutDirection direction = RR32Can::TurnoutDirection::GREEN;
  constexpr static const bool power = true;

  RR32Can::MachineTurnoutAddress turnout{GetParam()};
  RR32Can::CanFrame canFrame;
  hal::I2CMessage_t i2cMessage;
};

TEST_P(TurnoutRoutingFixture, Turnout_I2CtoCANandLocoNet) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));
  EXPECT_CALL(
      lnHal, requestSwitch(RR32Can::HumanTurnoutAddress(turnout.getNumericAddress()).value(), power,
                           RR32Can::TurnoutDirectionToIntegral(direction)));

  EXPECT_CALL(canHal, getCanMessage())
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
  EXPECT_CALL(lnHal, receive).WillOnce(Return(nullptr));

  // Inject I2C message
  EXPECT_CALL(i2cHal, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{&i2cMessage, hal::i2cRxDeleter})))
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));

  // Run!
  routingTask.loop();
}

TEST_P(TurnoutRoutingFixture, Turnout_CANtoI2CandLocoNet) {}

TEST_P(TurnoutRoutingFixture, Turnout_LocoNetToI2CandCAN) {}

INSTANTIATE_TEST_SUITE_P(TurnoutTest, TurnoutRoutingFixture, Values(0, 1, 5, 10, 42, 100, 255));

}  // namespace RoutingTask
}  // namespace tasks