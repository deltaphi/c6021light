#ifndef __MOCKS__SEQUENCEMAKER_H__
#define __MOCKS__SEQUENCEMAKER_H__

#include <algorithm>

#include "RR32Can/messages/CanFrame.h"

using namespace ::testing;

namespace mocks {

void makeSequence(hal::CANHalMock& mock) {
  EXPECT_CALL(mock, getCanMessage())
      .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
}

template <typename Message_t, size_t numMessages>
void makeSequence(hal::CANHalMock& mock, Message_t (&messages)[numMessages]) {
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

}  // namespace mocks

#endif  // __MOCKS__SEQUENCEMAKER_H__
