#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/RR32Can.h"
#include "mocks/RoutingForwarderMock.h"

namespace tasks {
namespace RoutingTask {

class StatelessRoutingFixture : public ::testing::Test {
 public:
  void SetUp() {
    RR32Can::Station::CallbackStruct callbacks;
    callbacks.tx = &canTx;
    RR32Can::RR32Can.begin(0, callbacks);
  }

  ::testing::StrictMock<CanTxMock> canTx;
};

TEST_F(StatelessRoutingFixture, Turnout_I2CtoCANandLocoNet) {}

TEST_F(StatelessRoutingFixture, Turnout_CANtoI2CandLocoNet) {}

TEST_F(StatelessRoutingFixture, Turnout_LocoNetToCandLocoNet) {}

}  // namespace RoutingTask
}  // namespace tasks