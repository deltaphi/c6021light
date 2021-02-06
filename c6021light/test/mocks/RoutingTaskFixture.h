#ifndef __MOCKS__ROUTINGTASKFIXTURE_H__
#define __MOCKS__ROUTINGTASKFIXTURE_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mocks/LocoNet.h"
#include "mocks/RoutingForwarderMock.h"

#include "RR32Can/RR32Can.h"

#include "tasks/RoutingTask/LocoNetHelpers.h"
#include "tasks/RoutingTask/RoutingTask.h"

using namespace ::testing;

namespace mocks {

/*
 * \brief Class RoutingTaskFixture
 */

class RoutingTaskFixture : public Test {
 public:
  void SetUp() {
    mocks::LocoNetInstance = &lnHal;
    hal::canMock = &canHal;
    hal::i2cMock = &i2cHal;

    routingTask.begin(dataModel, lnTx);

    RR32Can::Station::CallbackStruct callbacks;
    callbacks.tx = &canTx;
    RR32Can::RR32Can.begin(0, callbacks);
  }

  void TearDown() {
    hal::canMock = nullptr;
    hal::i2cMock = nullptr;
    mocks::LocoNetInstance = nullptr;
  }

  DataModel dataModel;
  StrictMock<hal::I2CHalMock> i2cHal;
  StrictMock<hal::CANHalMock> canHal;
  StrictMock<hal::CanTxMock> canTx;
  StrictMock<mocks::LocoNetClass> lnHal;
  StrictMock<mocks::LocoNetTx> lnTx;
  tasks::RoutingTask::RoutingTask routingTask;
};

}  // namespace mocks

#endif  // __MOCKS__ROUTINGTASKFIXTURE_H__
