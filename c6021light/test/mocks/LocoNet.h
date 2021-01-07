#ifndef __MOCKS__LOCONET_H__
#define __MOCKS__LOCONET_H__

#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ln_opc.h"

#define LocoNet (*mocks::LocoNetInstance)

uint8_t getLnMsgSize(volatile lnMsg* Msg);

inline bool operator==(const lnMsg& left, const lnMsg& right) {
  if (left.data[0] == right.data[0]) {
    auto leftSize = getLnMsgSize(const_cast<lnMsg*>(&left));
    if (leftSize == getLnMsgSize(const_cast<lnMsg*>(&right))) {
      return memcmp(left.data, right.data, leftSize) == 0;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

namespace mocks {

/*
 * \brief Class LocoNet
 */
class LocoNetClass {
 public:
  MOCK_METHOD(void, send, (lnMsg*), ());
  MOCK_METHOD(void, reportPower, (uint8_t state), ());
  MOCK_METHOD(void, reportSensor, (uint16_t address, uint8_t state), ());
  MOCK_METHOD(void, sendLongAck, (uint8_t ucCode), ());
  MOCK_METHOD(lnMsg*, receive, (), ());
};

extern LocoNetClass* LocoNetInstance;

}  // namespace mocks

#endif  // __MOCKS__LOCONET_H__
