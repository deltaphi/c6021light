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

 private:
};

}  // namespace freertossupport

#endif  // __MOCKS__OSTASK_H__
