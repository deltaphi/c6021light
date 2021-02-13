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

inline void makeSequence(hal::CANHalMock& mock) {
  EXPECT_CALL(mock, getCanMessage())
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
}

inline void makeSequence(hal::CANHalMock& mock, RR32Can::CanFrame& frame) {
  EXPECT_CALL(mock, getCanMessage())
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{&frame, hal::canRxDeleter})))
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

inline void makeSequence(hal::I2CHalMock& mock) {
  EXPECT_CALL(mock, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{nullptr, hal::i2cRxDeleter})));
}

inline void makeSequence(hal::I2CHalMock& mock, hal::I2CMessage_t& msg) {
  EXPECT_CALL(mock, getI2CMessage())
      .WillOnce(Return(ByMove(hal::I2CRxMessagePtr_t{&msg, hal::i2cRxDeleter})))
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

//
// LocoNet sequencing
//

inline void makeSequence(mocks::LocoNetClass& mock) {
  EXPECT_CALL(mock, receive()).WillOnce(Return(nullptr));
}

inline void makeSequence(mocks::LocoNetClass& mock, lnMsg& msg) {
  EXPECT_CALL(mock, receive()).WillOnce(Return(&msg)).WillOnce(Return(nullptr));
}

template <size_t numMessages>
void makeSequence(mocks::LocoNetClass& mock, lnMsg (&messages)[numMessages]) {
  Sequence s_;

  for (size_t i = 0; i < numMessages; ++i) {
    EXPECT_CALL(mock, receive()).InSequence(s_).WillOnce(Return(&(messages[i])));
  }

  EXPECT_CALL(mock, receive()).InSequence(s_).WillOnce(Return(nullptr));
}

}  // namespace mocks

#endif  // __MOCKS__SEQUENCEMAKER_H__
