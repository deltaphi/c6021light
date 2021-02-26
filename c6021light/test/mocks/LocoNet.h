#ifndef __MOCKS__LOCONET_H__
#define __MOCKS__LOCONET_H__

#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "LocoNetTx.h"
#include "ln_opc.h"

#define LocoNet (*mocks::LocoNetInstance)

uint8_t getLnMsgSize(volatile lnMsg* Msg);

inline bool operator==(const lnMsg& left, const lnMsg& right) {
  if (left.data[0] == right.data[0]) {
    const auto leftSize = getLnMsgSize(const_cast<lnMsg*>(&left));
    const auto rightSize = getLnMsgSize(const_cast<lnMsg*>(&right));
    if (leftSize == rightSize) {
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
  MOCK_METHOD(lnMsg*, receive, (), ());
};

class LocoNetTx : public ::LocoNetTx {
 public:
  MOCK_METHOD(bool, DoAsyncSend, (lnMsg), ());
};

extern LocoNetClass* LocoNetInstance;

}  // namespace mocks

#endif  // __MOCKS__LOCONET_H__
