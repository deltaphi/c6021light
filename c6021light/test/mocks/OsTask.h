#include "gmock/gmock.h"
#include "gtest/gtest.h"

#ifndef __MOCKS__OSTASK_H__
#define __MOCKS__OSTASK_H__

namespace freertossupport {

/*
 * \brief Class OsTask
 */
class OsTask {
 public:
  MOCK_METHOD(void, waitForNotify, (), ());
  MOCK_METHOD(void, notify, (), ());

 private:
};

}  // namespace freertossupport

#endif  // __MOCKS__OSTASK_H__
