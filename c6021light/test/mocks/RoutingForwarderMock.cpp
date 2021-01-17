#include "mocks/RoutingForwarderMock.h"

namespace hal {

I2CHalMock* i2cMock = nullptr;
CANHalMock* canMock = nullptr;

void sendI2CMessage(const I2CMessage_t& msg) { i2cMock->sendI2CMessage(msg); }
hal::I2CRxMessagePtr_t getI2CMessage() { return i2cMock->getI2CMessage(); }
hal::StopGoRequest getStopGoRequest() { return i2cMock->getStopGoRequest(); }
hal::CanRxMessagePtr_t getCanMessage() { return canMock->getCanMessage(); }

}  // namespace hal
