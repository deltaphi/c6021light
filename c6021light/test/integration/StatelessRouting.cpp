#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mocks/RoutingForwarderMock.h"

namespace tasks {
namespace RoutingTask {

class StatelessRoutingFixture : public ::testing::Test {
 public:
};

TEST_F(StatelessRoutingFixture, Turnout_I2CtoCAN) {}

}  // namespace RoutingTask
}  // namespace tasks