#ifndef __MOCKS__LOCONET_H__
#define __MOCKS__LOCONET_H__

#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ln_opc.h"

#define LocoNet (*mocks::LocoNetInstance)

namespace mocks {

/*
 * \brief Class LocoNet
 */
class LocoNetClass {
 public:
  MOCK_METHOD(void, send, (lnMsg*), ());
  MOCK_METHOD(void, requestSwitch, (uint16_t address, uint8_t output, uint8_t direction), ());
  MOCK_METHOD(void, reportPower, (uint8_t state), ());
  MOCK_METHOD(void, reportSensor, (uint16_t address, uint8_t state), ());
  MOCK_METHOD(void, sendLongAck, (uint8_t ucCode), ());
  MOCK_METHOD(lnMsg*, receive, (), ());
};

extern LocoNetClass* LocoNetInstance;

}  // namespace mocks

uint8_t getLnMsgSize(volatile lnMsg* Msg);

#endif  // __MOCKS__LOCONET_H__
