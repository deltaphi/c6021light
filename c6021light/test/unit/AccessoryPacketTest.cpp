#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MarklinI2C/Messages/AccessoryMsg.h"
#include "RR32Can/util/constexpr.h"
#include "mocks/KeyboardMock.h"

namespace MarklinI2C {
namespace Messages {

TEST(AccessoryPacket, ReceivedPacket) {
  AccessoryMsg msg = makeInboundAccessoryMsg(RR32Can::MachineTurnoutAddress(0),
                                             RR32Can::TurnoutDirection::GREEN, true);
  EXPECT_EQ(msg.getTurnoutAddr().value(), 0);
  EXPECT_EQ(msg.getDirection(), RR32Can::TurnoutDirection::GREEN);
  EXPECT_TRUE(msg.getPower());
}

TEST(AccessoryPacket, SentPacket) {
  AccessoryMsg actual;
  actual.setTurnoutAddr(RR32Can::util::DCC_Turnout(0xAAAu));
  actual.setDirection(RR32Can::TurnoutDirection::GREEN);
  actual.setPower(true);

  AccessoryMsg expected = makeOutboundAccessoryMsg(RR32Can::MachineTurnoutAddress(0xAAAu),
                                                   RR32Can::TurnoutDirection::GREEN, true);

  EXPECT_EQ(actual, expected);
}

}  // namespace Messages
}  // namespace MarklinI2C