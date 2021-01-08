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
  mocks::makeSequence(lnHal);

  // Inject I2C message
  mocks::makeSequence(i2cHal, i2cMessage);

  // Run!
  routingTask.loop();
}

TEST_P(TurnoutRoutingFixture, TurnoutRequest_CANtoLocoNetAndI2C) {
  // Setup expectations
  EXPECT_CALL(lnHal, send(Pointee(LnPacket)));

  mocks::makeSequence(i2cHal);
  mocks::makeSequence(lnHal);

  // Inject CAN message
  mocks::makeSequence(canHal, canFrame);

  // Run!
  routingTask.loop();
}

TEST_P(TurnoutRoutingFixture, TurnoutRequest_LocoNetToCANandI2C) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));

  mocks::makeSequence(i2cHal);
  mocks::makeSequence(canHal);

  // Inject LocoNet message
  mocks::makeSequence(lnHal, LnPacket);

  // Run!
  routingTask.loop();
}

INSTANTIATE_TEST_SUITE_P(TurnoutTest, TurnoutRoutingFixture,
                         Values(MM2_Turnout(0u), MM2_Turnout(1u), MM2_Turnout(5u), MM2_Turnout(10u),
                                MM2_Turnout(42u), MM2_Turnout(100u), MM2_Turnout(255u)));

using SenorTestParam_t = std::tuple<RR32Can::MachineTurnoutAddress, RR32Can::SensorState>;

class SenorRoutingFixture : public StatelessRoutingFixture,
                            public WithParamInterface<SenorTestParam_t> {
 public:
  constexpr static const RR32Can::MachineTurnoutAddress ZeroAddress{0};
  const RR32Can::MachineTurnoutAddress sensor{std::get<0>(GetParam())};
  const RR32Can::SensorState newState{std::get<1>(GetParam())};
  RR32Can::CanFrame canFrame{RR32Can::util::S88Event(ZeroAddress, sensor, newState)};
  lnMsg LnPacket{Ln_Sensor(sensor, newState)};
};

const RR32Can::MachineTurnoutAddress SenorRoutingFixture::ZeroAddress;

TEST_P(SenorRoutingFixture, SensorRequest_CANtoLocoNet) {
  // Setup expectations
  EXPECT_CALL(lnHal, send(Pointee(LnPacket)));

  mocks::makeSequence(i2cHal);
  mocks::makeSequence(lnHal);

  // Inject CAN message
  mocks::makeSequence(canHal, canFrame);

  // Run!
  routingTask.loop();
}

TEST_P(SenorRoutingFixture, SensorRequest_LocoNetToCAN) {
  // Setup expectations
  canFrame.id.setResponse(true);
  EXPECT_CALL(canTx, SendPacket(canFrame));

  mocks::makeSequence(i2cHal);
  mocks::makeSequence(canHal);

  // Inject LocoNet message
  mocks::makeSequence(lnHal, LnPacket);

  // Run!
  routingTask.loop();
}

using Addr = RR32Can::MachineTurnoutAddress;

INSTANTIATE_TEST_SUITE_P(SensorTest, SenorRoutingFixture,
                         Combine(Values(Addr(0), Addr(1), Addr(2), Addr(3), Addr(4), Addr(10),
                                        Addr(100), Addr(255), Addr(0x7FF)),
                                 Values(RR32Can::SensorState::OPEN, RR32Can::SensorState::CLOSED)));

}  // namespace RoutingTask
}  // namespace tasks