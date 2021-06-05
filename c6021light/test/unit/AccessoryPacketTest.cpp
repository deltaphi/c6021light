#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/util/constexpr.h"

using namespace ::testing;
using namespace ::RR32Can::util;

namespace MarklinI2C {
namespace Messages {

using AccessoryPacketFixture_TestParam_t = RR32Can::MachineTurnoutAddress;

class AccessoryPacketFixture : public TestWithParam<AccessoryPacketFixture_TestParam_t> {
 public:
  RR32Can::MachineTurnoutAddress getTurnout() const { return GetParam(); }
};
TEST_P(AccessoryPacketFixture, ReceivedPacket) {
  AccessoryMsg msg =
      AccessoryMsg::makeInbound(getTurnout(), RR32Can::TurnoutDirection::GREEN, true);
  EXPECT_EQ(msg.getInboundTurnoutAddr().value(), getTurnout().value());
  EXPECT_EQ(msg.getDirection(), RR32Can::TurnoutDirection::GREEN);
  EXPECT_TRUE(msg.getPower());
}

INSTANTIATE_TEST_SUITE_P(AccessoryPacket, AccessoryPacketFixture,
                         Values(RR32Can::MachineTurnoutAddress(0u),
                                RR32Can::MachineTurnoutAddress(98u)));

TEST(AccessoryPacket, ReceivedPacket) {
  AccessoryMsg actual = AccessoryMsg::makeInbound(RR32Can::MachineTurnoutAddress(0xAAu),
                                                  RR32Can::TurnoutDirection::GREEN, true);
  AccessoryMsg expected;
  expected.source_ = 0x34;
  expected.destination_ = 0x7F;
  expected.data_ = 0x2D;

  EXPECT_EQ(actual, expected);
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

TEST(AccessoryPacket, Response) {
  AccessoryMsg inbound = AccessoryMsg::makeInbound(RR32Can::MachineTurnoutAddress(0xAAu),
                                                   RR32Can::TurnoutDirection::GREEN, true);
  AccessoryMsg outbound = AccessoryMsg::makeOutbound(inbound);

  AccessoryMsg expected;
  expected.source_ = 0x7F;
  expected.destination_ = 0x34;
  expected.data_ = 0x2D;

  EXPECT_EQ(outbound, expected);
}

}  // namespace Messages
}  // namespace MarklinI2C
