#ifndef __MOCKS__SEQUENCEMAKER_H__
#define __MOCKS__SEQUENCEMAKER_H__

#include <algorithm>

#include "RR32Can/messages/CanFrame.h"
#include "hal/stm32I2C.h"

using namespace ::testing;

namespace mocks {

//
// CAN sequencing
//

void makeSequence(hal::CANHalMock& mock) {
  EXPECT_CALL(mock, getCanMessage())
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
}

template <size_t numMessages>
void makeSequence(hal::CANHalMock& mock, RR32Can::CanFrame (&messages)[numMessages]) {
  Sequence s_;

  for (size_t i = 0; i < numMessages; ++i) {
    EXPECT_CALL(mock, getCanMessage())
        .InSequence(s_)
        .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{&(messages[i]), hal::canRxDeleter})));
  }

  EXPECT_CALL(mock, getCanMessage())
      .InSequence(s_)
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
}

//
// I2C sequencing
//

void makeSequence(hal::I2CHalMock& mock) {
  EXPECT_CALL(mock, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));
}

template <size_t numMessages>
void makeSequence(hal::I2CHalMock& mock, hal::I2CMessage_t (&messages)[numMessages]) {
  Sequence s_;

  for (size_t i = 0; i < numMessages; ++i) {
    EXPECT_CALL(mock, getI2CMessage())
        .InSequence(s_)
        .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{&(messages[i]), hal::i2cRxDeleter})));
  }

  EXPECT_CALL(mock, getI2CMessage())
      .InSequence(s_)
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));
}

}  // namespace mocks

#endif  // __MOCKS__SEQUENCEMAKER_H__
