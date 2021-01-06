#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "mocks/KeyboardMock.h"

namespace MarklinI2C {
namespace Messages {

TEST(AccessoryPacket, ReceivedPacket) {
  AccessoryMsg msg = mocks::makeReceivedAccessoryMsg(RR32Can::MachineTurnoutAddress(0),
                                                     RR32Can::TurnoutDirection::GREEN, true);
  EXPECT_EQ(msg.getTurnoutAddr().value(), 0);
  EXPECT_EQ(msg.getDirection(), RR32Can::TurnoutDirection::GREEN);
  EXPECT_TRUE(msg.getPower());
}

}  // namespace Messages
}  // namespace MarklinI2C