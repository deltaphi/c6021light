#ifndef __MOCKS__LOCONET_H__
#define __MOCKS__LOCONET_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ln_opc.h"

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

 private:
};

}  // namespace mocks

// Copies from LocoNet library for the purpose of stubbing
inline uint8_t getLnMsgSize(volatile lnMsg* Msg) {
  return ((Msg->sz.command & (uint8_t)0x60) == (uint8_t)0x60)
             ? Msg->sz.mesg_size
             : ((Msg->sz.command & (uint8_t)0x60) >> (uint8_t)4) + 2;
}

extern mocks::LocoNetClass LocoNet;

#endif  // __MOCKS__LOCONET_H__
