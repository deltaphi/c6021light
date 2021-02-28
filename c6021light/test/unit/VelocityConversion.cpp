#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "tasks/RoutingTask/LocoNetHelpers.h"

using namespace ::tasks::RoutingTask;
using namespace ::testing;

using TestParam_t = std::tuple<RR32Can::Velocity_t, uint8_t>;

class VelocityConversionFixture : public TestWithParam<TestParam_t> {
 public:
  const RR32Can::Velocity_t canVelocity = std::get<0>(GetParam());
  const uint8_t lnSpeed = std::get<1>(GetParam());
};

TEST_P(VelocityConversionFixture, CanLn) {
  const auto convertedLnSpeed = canVelocityToLnSpeed(canVelocity);
  EXPECT_EQ(convertedLnSpeed, lnSpeed);
}

TEST_P(VelocityConversionFixture, LnCan) {
  const auto convertedCanVelocity = lnSpeedToCanVelocity(lnSpeed);
  EXPECT_EQ(convertedCanVelocity, canVelocity);
}

INSTANTIATE_TEST_SUITE_P(VelocityConversion, VelocityConversionFixture,
                         Values(TestParam_t{0, 0}, TestParam_t{496, 63}, TestParam_t{504, 64},
                                TestParam_t{1000, 127}));
