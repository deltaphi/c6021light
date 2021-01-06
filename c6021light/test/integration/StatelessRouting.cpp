#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mocks/LocoNet.h"
#include "mocks/RoutingForwarderMock.h"

#include "RR32Can/RR32Can.h"

#include "tasks/RoutingTask/RoutingTask.h"

using namespace ::testing;

mocks::LocoNetClass LocoNet;

namespace tasks {
namespace RoutingTask {

class StatelessRoutingFixture : public Test {
 public:
  void SetUp() {
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
  }

  DataModel dataModel;
  StrictMock<hal::I2CHalMock> i2cHal;
  StrictMock<hal::CANHalMock> canHal;
  StrictMock<hal::CanTxMock> canTx;
  RoutingTask routingTask;
};

class TurnoutRoutingFixture : public StatelessRoutingFixture {
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
    {
      i2cMessage.destination_ = MarklinI2C::kCentralAddr;
      i2cMessage.setTurnoutAddr(turnout.getNumericAddress());
      i2cMessage.setDirection(direction);
      i2cMessage.setPower(power);
      i2cMessage.makePowerConsistent();
    }
  }

  constexpr static const RR32Can::TurnoutDirection direction = RR32Can::TurnoutDirection::GREEN;
  constexpr static const bool power = true;

  RR32Can::MachineTurnoutAddress turnout{0x02};
  RR32Can::CanFrame canFrame;
  hal::I2CMessage_t i2cMessage;
};

TEST_F(TurnoutRoutingFixture, Turnout_I2CtoCANandLocoNet) {
  // Setup expectations
  EXPECT_CALL(canTx, SendPacket(canFrame));
  EXPECT_CALL(LocoNet,
              requestSwitch(RR32Can::HumanTurnoutAddress(turnout.getNumericAddress()).value(),
                            power, RR32Can::TurnoutDirectionToIntegral(direction)));

  EXPECT_CALL(canHal, getCanMessage())
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
  EXPECT_CALL(LocoNet, receive).WillOnce(Return(nullptr));

  // Inject I2C message
  EXPECT_CALL(i2cHal, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{&i2cMessage, hal::i2cRxDeleter})))
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));

  // Run!
  routingTask.loop();
}

TEST_F(TurnoutRoutingFixture, Turnout_CANtoI2CandLocoNet) {}

TEST_F(TurnoutRoutingFixture, Turnout_LocoNetToI2CandCAN) {}

}  // namespace RoutingTask
}  // namespace tasks