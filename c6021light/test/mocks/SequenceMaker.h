#ifndef __MOCKS__SEQUENCEMAKER_H__
#define __MOCKS__SEQUENCEMAKER_H__

#include <algorithm>

#include "RR32Can/messages/CanFrame.h"

using namespace ::testing;

namespace mocks {

class NullCanSequence {
 public:
  NullCanSequence() = default;
  NullCanSequence(hal::CANHalMock& mock) { expectNull(mock); }

  const NullCanSequence& expectNull(hal::CANHalMock& mock) const {
    EXPECT_CALL(mock, getCanMessage())
        .InSequence(s_)
        .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
    return *this;
  }

  const NullCanSequence& expectNull(hal::CANHalMock& mock, std::size_t count) const {
    for (std::size_t i{0}; i < count; ++i) {
      EXPECT_CALL(mock, getCanMessage())
          .InSequence(s_)
          .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter})));
    }
    return *this;
  }

  void expectNullInfinite(hal::CANHalMock& mock) const {
    EXPECT_CALL(mock, getCanMessage()).InSequence(s_).WillRepeatedly(Invoke([]() {
      return hal::CanRxMessagePtr_t{nullptr, hal::canRxDeleter};
    }));
  }

 protected:
  Sequence s_;
};

template <size_t numMessages>
class CanSequence : public NullCanSequence {
 public:
  constexpr CanSequence(const RR32Can::CanFrame messages[numMessages]) : messages_(messages) {}
  CanSequence(hal::CANHalMock& mock, const RR32Can::CanFrame messages[numMessages]) {
    std::copy(messages, messages + numMessages, messages_);
    setExpectation(mock);
    expectNull(mock);
  }

  CanSequence& setExpectation(hal::CANHalMock& mock) {
    for (std::size_t i{0}; i < numMessages; ++i) {
      EXPECT_CALL(mock, getCanMessage())
          .InSequence(s_)
          .WillOnce(Return(ByMove(hal::CanRxMessagePtr_t{&messages_[i], hal::canRxDeleter})));
    }
    return *this;
  }

 private:
  RR32Can::CanFrame messages_[numMessages];
};

}  // namespace mocks

#endif  // __MOCKS__SEQUENCEMAKER_H__
