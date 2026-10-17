#ifndef __MOCKS__ROUTINGTASKFIXTURE_H__
#define __MOCKS__ROUTINGTASKFIXTURE_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mocks/LocoNet.h"
#include "mocks/RoutingForwarderMock.h"
#include "mocks/StatusIndicator.h"
#include "mocks/XpressNetMaster.h"

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
    mocks::XpressNetMasterInstance = &xnHal;
    hal::canMock = &canHal;
    hal::i2cMock = &i2cHal;

    routingTask.begin(dataModel, lnTx, stopGoTimer, canEngineDBTimer, statusIndicator);

    RR32Can::Station::CallbackStruct callbacks;
    callbacks.tx = &canTx;
    callbacks.engine = &routingTask.getCANEngineDB();
    RR32Can::RR32Can.begin(0, callbacks);
  }

  void TearDown() {
    hal::canMock = nullptr;
    hal::i2cMock = nullptr;
    mocks::LocoNetInstance = nullptr;
    mocks::XpressNetMasterInstance = nullptr;
  }

  DataModel dataModel;
  StrictMock<mocks::StatusIndicator> statusIndicator;
  StrictMock<hal::I2CHalMock> i2cHal;
  StrictMock<hal::CANHalMock> canHal;
  StrictMock<hal::CanTxMock> canTx;
  StrictMock<mocks::LocoNetClass> lnHal;
  StrictMock<mocks::LocoNetTx> lnTx;
  StrictMock<mocks::XpressNetMasterClass> xnHal;
  tasks::RoutingTask::RoutingTask routingTask;
  StrictMock<freertossupport::OsTimer> stopGoTimer;
  StrictMock<freertossupport::OsTimer> canEngineDBTimer;
};

}  // namespace mocks

#endif  // __MOCKS__ROUTINGTASKFIXTURE_H__
