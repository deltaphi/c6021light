#ifndef __MOCKS__ROUTINGFORWARDERMOCK_H__
#define __MOCKS__ROUTINGFORWARDERMOCK_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RR32Can/callback/TxCbk.h"

#include "hal/stm32I2C.h"
#include "hal/stm32can.h"

namespace hal {

class CanTxMock : public RR32Can::callback::TxCbk {
 public:
  MOCK_METHOD(void, SendPacket, (const RR32Can::CanFrame& canFrame), (override));
};

class I2CHalMock {
 public:
  MOCK_METHOD(void, sendI2CMessage, (const hal::I2CMessage_t& msg), ());
  MOCK_METHOD(hal::I2CRxMessagePtr_t, getI2CMessage, (), ());
  MOCK_METHOD(hal::StopGoRequest, getStopGoRequest, (), ());
};

class CANHalMock {
 public:
  MOCK_METHOD(hal::CanRxMessagePtr_t, getCanMessage, (), ());
};

inline void canRxDeleter(RR32Can::CanFrame*) {
  // Empty deleter
}

inline void i2cRxDeleter(I2CMessage_t*) {
  // Empty deleter
}

extern I2CHalMock* i2cMock;
extern CANHalMock* canMock;

}  // namespace hal

#endif  // __MOCKS__ROUTINGFORWARDERMOCK_H__
