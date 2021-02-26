#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "tasks/RoutingTask/LocoNetSlotServer.h"

#include "mocks/RoutingTaskFixture.h"

#include "mocks/SequenceMaker.h"

class EngineRoutingFixture : public mocks::RoutingTaskFixture {
 public:
  void SetUp() {
    mocks::RoutingTaskFixture::SetUp();

    exampleLoco_.setUid(0x101010);
    exampleLoco_.setAddress(RR32Can::util::MM2_Loco(42U));
    exampleLoco_.setName("BR 42");
    exampleLoco_.setAvailability(RR32Can::LocomotiveShortInfo::AvailabilityStatus::FULL_DETAILS);

    RR32Can::Station::CallbackStruct callbacks;
    callbacks.engine = &routingTask.getCANEngineDB();
    RR32Can::RR32Can.begin(0, callbacks);

    dataModel.lnSlotServerState = tasks::RoutingTask::LocoNetSlotServer::SlotServerState::PASSIVE;

    {
      auto slotIt =
          routingTask.getLnSlotServer().findOrAllocateSlotForAddress(exampleLoco_.getAddress());
      ASSERT_NE(slotIt, routingTask.getLnSlotServer().end());
      slotIt->loco = exampleLoco_;
      slotIt->inUse = true;
      slotIt->needsMatchToCAN = true;
    }

    {
      auto engineIt = routingTask.getCANEngineDB().begin();
      engineIt->loco = exampleLoco_;
    }
  }

  RR32Can::Locomotive exampleLoco_;
  const uint8_t kLocoSlotIdx = 1;
};

namespace tasks {
namespace RoutingTask {

TEST_F(EngineRoutingFixture, SpeedChangeCanToLn) {
  RR32Can::CanFrame injectedMessage = RR32Can::util::LocoSpeed(false, exampleLoco_.getUid(), 500);
  const lnMsg expectedMessage = Ln_LocoSpeed(1, 500);

  mocks::makeSequence(i2cHal);
  EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
  mocks::makeSequence(lnHal);
  mocks::makeSequence(canHal, injectedMessage);

  EXPECT_CALL(lnTx, DoAsyncSend(expectedMessage));

  routingTask.loop();
}

}  // namespace RoutingTask
}  // namespace tasks