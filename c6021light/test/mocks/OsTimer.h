#include "gmock/gmock.h"
#include "gtest/gtest.h"

#ifndef __MOCKS__OSTIMER_H__
#define __MOCKS__OSTIMER_H__

using TimerHandle_t = uint8_t;
using TickType_t = uint32_t;

namespace freertossupport {

class TimerCallbackBase {
 public:
  virtual void TimerCallback(TimerHandle_t) = 0;
};

/*
 * \brief Class OsTimer
 */
class OsTimer {
 public:
  MOCK_METHOD(void, Start, (), ());
  MOCK_METHOD(void, Stop, (), ());
  TimerHandle_t getHandle() const { return 0; }

 private:
};

}  // namespace freertossupport

#endif  // __MOCKS__OSTIMER_H__
