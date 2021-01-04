#ifndef __MOCKS__LOCONET_H__
#define __MOCKS__LOCONET_H__

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ln_opc.h"

namespace mocks {

/*
 * \brief Class LocoNet
 */    
class LocoNetClass {
  public:
  MOCK_METHOD(void, send, (lnMsg*), ())
  private:
};

}  // namespace mocks

#endif  // __MOCKS__LOCONET_H__
