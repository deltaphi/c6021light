#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/util/constexpr.h"

namespace MarklinI2C {
namespace Messages {

TEST(AccessoryPacket, ReceivedPacket) {
  AccessoryMsg msg = AccessoryMsg::makeInbound(RR32Can::MachineTurnoutAddress(0),
                                               RR32Can::TurnoutDirection::GREEN, true);
  EXPECT_EQ(msg.getTurnoutAddr().value(), 0);
  EXPECT_EQ(msg.getDirection(), RR32Can::TurnoutDirection::GREEN);
  EXPECT_TRUE(msg.getPower());
}

TEST(AccessoryPacket, SentPacket) {
  AccessoryMsg actual = AccessoryMsg::makeOutbound(RR32Can::MachineTurnoutAddress(0xAAu),
                                                   RR32Can::TurnoutDirection::GREEN, true);

  AccessoryMsg expected;
  expected.source_ = 0x7F;
  expected.destination_ = 0x34;
  expected.data_ = 0x2D;

  EXPECT_EQ(actual, expected);
}

}  // namespace Messages
}  // namespace MarklinI2C