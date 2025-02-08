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

TEST(I2CForwarderTest, Remap_Umapped_Remapped_Unmapped) {
  auto i2cForwarder = tasks::RoutingTask::I2CForwarder();
  DataModel model;
  i2cForwarder.init(model);

  const RR32Can::MachineTurnoutAddress turnoutAddress{0xAAu};
  const RR32Can::MachineTurnoutAddress remappedAddress{0xBBu};

  {
    const RR32Can::MachineTurnoutAddress actual{i2cForwarder.remapTurnoutAddress(turnoutAddress)};
    EXPECT_EQ(actual, turnoutAddress);
  }
  model.i2cTurnoutMap.insert({turnoutAddress, remappedAddress});
  {
    const RR32Can::MachineTurnoutAddress actual{i2cForwarder.remapTurnoutAddress(turnoutAddress)};
    EXPECT_EQ(actual, remappedAddress);
  }
  model.i2cTurnoutMap.erase(turnoutAddress);
  {
    const RR32Can::MachineTurnoutAddress actual{i2cForwarder.remapTurnoutAddress(turnoutAddress)};
    EXPECT_EQ(actual, turnoutAddress);
  }
}
