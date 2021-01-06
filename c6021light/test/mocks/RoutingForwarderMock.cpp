#include "mocks/RoutingForwarderMock.h"

namespace hal {

I2CHalMock* i2cMock;
CANHalMock* canMock;

void sendI2CMessage(const I2CMessage_t& msg) { i2cMock->sendI2CMessage(msg); }

}  // namespace hal
