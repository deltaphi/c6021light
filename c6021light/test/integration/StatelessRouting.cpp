#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/messages/S88Event.h"
#include "RR32Can/util/constexpr.h"

#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

using namespace ::testing;
using namespace ::RR32Can::util;

namespace tasks {
namespace RoutingTask {

using TurnoutTestParam_t = RR32Can::MachineTurnoutAddress;

class TurnoutRoutingFixture : public mocks::RoutingTaskFixture,
                              public WithParamInterface<TurnoutTestParam_t> {
 public:
  constexpr static const RR32Can::TurnoutDirection direction = RR32Can::TurnoutDirection::GREEN;
  constexpr static const bool power = true;

  void SetUp() {
    mocks::RoutingTaskFixture::SetUp();
    EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  }

  RR32Can::MachineTurnoutAddress turnout{GetParam()};
  RR32Can::CanFrame canFrame{Turnout(false, turnout, direction, power)};
  hal::I2CMessage_t i2cMessage{
      MarklinI2C::Messages::AccessoryMsg::makeInbound(turnout, direction, power)};
  lnMsg LnPacket{Ln_Turnout(turnout, direction, power)};
};

TEST_P(TurnoutRoutingFixture, TurnoutRequest_I2CtoCANandLocoNet) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));
  EXPECT_CALL(lnTx, DoAsyncSend(LnPacket));

  mocks::makeSequence(canHal);
  mocks::makeSequence(lnHal);

  // Inject I2C message
  mocks::makeSequence(i2cHal, i2cMessage);

  // Run!
  routingTask.loop();
}

TEST_P(TurnoutRoutingFixture, TurnoutRequest_CANtoLocoNetAndI2C) {
  // Setup expectations
  EXPECT_CALL(lnTx, DoAsyncSend(LnPacket));

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

TEST_P(TurnoutRoutingFixture, TurnoutRequest_I2CtoCANandLocoNet_WithGeneratedResponse) {
  // Configure data model
  dataModel.generateI2CTurnoutResponse = true;

  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));
  hal::I2CMessage_t i2cResponseMessage =
      MarklinI2C::Messages::AccessoryMsg::makeOutbound(turnout, direction, power);
  EXPECT_CALL(i2cHal, sendI2CMessage(i2cResponseMessage));
  EXPECT_CALL(lnTx, DoAsyncSend(LnPacket));

  mocks::makeSequence(canHal);
  mocks::makeSequence(lnHal);

  // Inject I2C message
  mocks::makeSequence(i2cHal, i2cMessage);

  // Run!
  routingTask.loop();
}

INSTANTIATE_TEST_SUITE_P(TurnoutTest, TurnoutRoutingFixture,
                         Values(MM2_Turnout(0u), MM2_Turnout(1u), MM2_Turnout(5u), MM2_Turnout(10u),
                                MM2_Turnout(42u), MM2_Turnout(100u), MM2_Turnout(255u)));

class TurnoutRoutingWithRemappingFixture : public mocks::RoutingTaskFixture {
 public:
  constexpr static const RR32Can::TurnoutDirection direction = RR32Can::TurnoutDirection::GREEN;
  constexpr static const bool power = true;

  void SetUp() {
    mocks::RoutingTaskFixture::SetUp();
    EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));

    // Configure data model
    dataModel.generateI2CTurnoutResponse = true;
    dataModel.accessoryRailProtocol = RR32Can::RailProtocol::MM2;
    dataModel.i2cTurnoutMap.insert({turnoutButton, turnoutRemapped});
  }

  // Turnout should bey keyboard 2, switch 14 -> 16+14 = human(30)
  RR32Can::MachineTurnoutAddress turnoutButton{RR32Can::HumanTurnoutAddress{30}};

  // Address is remapped to Keyboard 3, switch 4 -> 16+16+4 = human(36);
  RR32Can::MachineTurnoutAddress turnoutRemapped{RR32Can::HumanTurnoutAddress{36}};

  RR32Can::CanFrame canFrame{Turnout(false, MM2_Turnout(turnoutRemapped), direction, power)};
  hal::I2CMessage_t i2cMessage{
      MarklinI2C::Messages::AccessoryMsg::makeInbound(turnoutButton, direction, power)};
  lnMsg LnPacket{Ln_Turnout(turnoutRemapped, direction, power)};
};

TEST_F(TurnoutRoutingWithRemappingFixture, Remapped_On) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));
  hal::I2CMessage_t i2cResponseMessage =
      MarklinI2C::Messages::AccessoryMsg::makeOutbound(turnoutButton, direction, power);
  EXPECT_CALL(i2cHal, sendI2CMessage(i2cResponseMessage));
  EXPECT_CALL(lnTx, DoAsyncSend(LnPacket));

  mocks::makeSequence(canHal);
  mocks::makeSequence(lnHal);

  // Inject I2C message
  mocks::makeSequence(i2cHal, i2cMessage);

  // Run!
  routingTask.loop();
}

using SensorTestParam_t = std::tuple<RR32Can::MachineTurnoutAddress, RR32Can::SensorState>;

class SensorRoutingFixture : public mocks::RoutingTaskFixture,
                            public WithParamInterface<SensorTestParam_t> {
 public:
  void SetUp() {
    mocks::RoutingTaskFixture::SetUp();
    EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  }

  constexpr static const RR32Can::MachineTurnoutAddress ZeroAddress{0};
  const RR32Can::MachineTurnoutAddress sensor{std::get<0>(GetParam())};
  const RR32Can::SensorState newState{std::get<1>(GetParam())};
  RR32Can::CanFrame canFrame{RR32Can::util::S88Event(ZeroAddress, sensor, newState)};
  lnMsg LnPacket{Ln_Sensor(sensor, newState)};
};

const RR32Can::MachineTurnoutAddress SensorRoutingFixture::ZeroAddress;

TEST_P(SensorRoutingFixture, SensorRequest_CANtoLocoNet) {
  // Setup expectations
  EXPECT_CALL(lnTx, DoAsyncSend(LnPacket));

  mocks::makeSequence(i2cHal);
  mocks::makeSequence(lnHal);

  // Inject CAN message
  mocks::makeSequence(canHal, canFrame);

  // Run!
  routingTask.loop();
}

TEST_P(SensorRoutingFixture, SensorRequest_LocoNetToCAN) {
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

INSTANTIATE_TEST_SUITE_P(SensorTest, SensorRoutingFixture,
                         Combine(Values(Addr(0), Addr(1), Addr(2), Addr(3), Addr(4), Addr(10),
                                        Addr(100), Addr(255), Addr(0x7FF)),
                                 Values(RR32Can::SensorState::OPEN, RR32Can::SensorState::CLOSED)));

using PowerTestParam_t = bool;

class PowerRoutingFixture : public mocks::RoutingTaskFixture,
                            public WithParamInterface<PowerTestParam_t> {
 public:
  void SetUp() {
    mocks::RoutingTaskFixture::SetUp();
    if (power) {
      canFrame = RR32Can::util::System_Go(false);
      LnPacket = Ln_On();
      stopGoRequest.goRequest = true;
    } else {
      canFrame = RR32Can::util::System_Stop(false);
      LnPacket = Ln_Off();
      stopGoRequest.stopRequest = true;
    }
  }
  const bool power{GetParam()};
  RR32Can::CanFrame canFrame{};
  lnMsg LnPacket{};
  hal::StopGoRequest stopGoRequest;
};

TEST_P(PowerRoutingFixture, PowerRequest_CANtoLocoNet) {
  // Setup expectations
  EXPECT_CALL(lnTx, DoAsyncSend(LnPacket));

  mocks::makeSequence(i2cHal);
  mocks::makeSequence(lnHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));

  // Inject CAN message
  mocks::makeSequence(canHal, canFrame);

  // Run!
  routingTask.loop();
}

TEST_P(PowerRoutingFixture, PowerResponse_CANtoLocoNet) {
  // Setup expectations
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(i2cHal);
  mocks::makeSequence(lnHal);

  // Inject CAN message
  canFrame.id.setResponse(true);
  mocks::makeSequence(canHal, canFrame);

  // Run!
  routingTask.loop();
}

TEST_P(PowerRoutingFixture, PowerRequest_LocoNetToCAN) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));

  mocks::makeSequence(i2cHal);
  mocks::makeSequence(canHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));

  // Inject LocoNet message
  mocks::makeSequence(lnHal, LnPacket);

  // Run!
  routingTask.loop();
}

TEST_P(PowerRoutingFixture, PowerRequest_FromI2C) {
  // Expoected output packets
  EXPECT_CALL(canTx, SendPacket(canFrame));
  EXPECT_CALL(lnTx, DoAsyncSend(LnPacket));

  // Expected input packets
  mocks::makeSequence(i2cHal);
  mocks::makeSequence(canHal);
  mocks::makeSequence(lnHal);

  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(stopGoRequest));

  // Run!
  routingTask.loop();
}

INSTANTIATE_TEST_SUITE_P(PowerTest, PowerRoutingFixture, Values(true, false));

}  // namespace RoutingTask
}  // namespace tasks
