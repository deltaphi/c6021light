#ifndef __MOCKS__STATUSINDICATOR_H__
#define __MOCKS__STATUSINDICATOR_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "IStatusIndicator.h"

namespace mocks {

/*
 * \brief Class StatusIndicator
 */
class StatusIndicator : public IStatusIndicator {
 public:
  MOCK_METHOD(void, setError, (), (override));
  MOCK_METHOD(void, clearError, (), (override));

  MOCK_METHOD(void, setCanDbDownload, (), (override));
  MOCK_METHOD(void, clearCanDbDownload, (), (override));

 private:
};

}  // namespace mocks

#endif  // __MOCKS__STATUSINDICATOR_H__
