#ifndef __MOCKS__ROUTINGFORWARDERMOCK_H__
#define __MOCKS__ROUTINGFORWARDERMOCK_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/callback/TxCbk.h"

#include "hal/stm32can.h"
#include "hal/stm32i2c.h"

namespace hal {

class CanTxMock : public RR32Can::callback::TxCbk {
 public:
  MOCK_METHOD(void, SendPacket, (const RR32Can::CanFrame& canFrame), (override));
};

class I2CHalMock {
 public:
  MOCK_METHOD(void, sendI2CMessage, (const hal::I2CMessage_t& msg), ());
};

class CANHalMock {};

extern I2CHalMock* i2cMock;
extern CANHalMock* canMock;

}  // namespace hal

#endif  // __MOCKS__ROUTINGFORWARDERMOCK_H__
