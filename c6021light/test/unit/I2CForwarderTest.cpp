#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "tasks/RoutingTask/I2CForwarder.h"

class I2CForwarderFixture: public ::testing::Test {
public:
  
  DataModel model;
  tasks::RoutingTask::I2CForwarder i2cForwarder;
  
  void SetUp() {
    i2cForwarder.init(model);

  }

};

TEST_F(I2CForwarderFixture, Remap_Unmapped) {
  const RR32Can::MachineTurnoutAddress turnoutAddress{0xAAu};
  const RR32Can::MachineTurnoutAddress actual{i2cForwarder.remapTurnoutAddress(turnoutAddress)};

  EXPECT_EQ(actual, turnoutAddress);
}

TEST_F(I2CForwarderFixture, Remap_Remapped) {
  const RR32Can::MachineTurnoutAddress turnoutAddress{0xAAu};
  const RR32Can::MachineTurnoutAddress expected{0xBBu};

  model.i2cTurnoutMap.insert({turnoutAddress, expected});

  const RR32Can::MachineTurnoutAddress actual{i2cForwarder.remapTurnoutAddress(turnoutAddress)};

  EXPECT_EQ(actual, expected);
}

TEST_F(I2CForwarderFixture, Remap_Umapped_Remapped_Unmapped) {
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
