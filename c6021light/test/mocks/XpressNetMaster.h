#ifndef __MOCKS__XPRESSNETMASTER_H__
#define __MOCKS__XPRESSNETMASTER_H__

#include <functional>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define XpressNet (*mocks::XpressNetMasterInstance)

#define csNormal 0x00
#define csTrackVoltageOff 0x02

namespace mocks {

/*
 * \brief Class XpressNetMaster
 */
class XpressNetMasterClass {
 public:
  MOCK_METHOD(void, setPower, (uint8_t Power), ());
  MOCK_METHOD(void, SetTrntPos, (uint16_t Address, uint8_t state, uint8_t active), ());
};

extern XpressNetMasterClass* XpressNetMasterInstance;

}  // namespace mocks

void notifyXNetGlobal(void);

#endif  // __MOCKS__XPRESSNETMASTER_H__
