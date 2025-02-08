#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "tasks/RoutingTask/I2CForwarder.h"

TEST(I2CForwarderTest, Remap_Unmapped) {
  auto i2cForwarder = tasks::RoutingTask::I2CForwarder();
  DataModel model;
  i2cForwarder.init(model);

  const RR32Can::MachineTurnoutAddress turnoutAddress{0xAAu};
  const RR32Can::MachineTurnoutAddress actual{i2cForwarder.remapTurnoutAddress(turnoutAddress)};

  EXPECT_EQ(actual, turnoutAddress);
}

TEST(I2CForwarderTest, Remap_Remapped) {
  auto i2cForwarder = tasks::RoutingTask::I2CForwarder();
  DataModel model;
  i2cForwarder.init(model);

  const RR32Can::MachineTurnoutAddress turnoutAddress{0xAAu};
  const RR32Can::MachineTurnoutAddress expected{0xBBu};

  model.i2cTurnoutMap.insert({turnoutAddress, expected});

  const RR32Can::MachineTurnoutAddress actual{i2cForwarder.remapTurnoutAddress(turnoutAddress)};

  EXPECT_EQ(actual, expected);
}
