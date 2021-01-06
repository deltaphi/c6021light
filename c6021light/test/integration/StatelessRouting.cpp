#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mocks/LocoNet.h"
#include "mocks/RoutingForwarderMock.h"

#include "RR32Can/RR32Can.h"

#include "tasks/RoutingTask/RoutingTask.h"

mocks::LocoNetClass LocoNet;

namespace tasks {
namespace RoutingTask {

class StatelessRoutingFixture : public ::testing::Test {
 public:
  void SetUp() {
    hal::canMock = &canHal;
    hal::i2cMock = &i2cHal;

    RR32Can::Station::CallbackStruct callbacks;
    callbacks.tx = &canTx;
    RR32Can::RR32Can.begin(0, callbacks);
  }

  ::testing::StrictMock<hal::I2CHalMock> i2cHal;
  ::testing::StrictMock<hal::CANHalMock> canHal;
  ::testing::StrictMock<hal::CanTxMock> canTx;
  RoutingTask routingTask;
};

TEST_F(StatelessRoutingFixture, Turnout_I2CtoCANandLocoNet) {}

TEST_F(StatelessRoutingFixture, Turnout_CANtoI2CandLocoNet) {}

TEST_F(StatelessRoutingFixture, Turnout_LocoNetToCandLocoNet) {}

}  // namespace RoutingTask
}  // namespace tasks