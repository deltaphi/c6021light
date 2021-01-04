#ifndef __MOCKS__ROUTINGFORWARDERMOCK_H__
#define __MOCKS__ROUTINGFORWARDERMOCK_H__

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "tasks/RoutingTask/RoutingForwarder.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class RoutingForwarderMock
 */
class RoutingForwarderMock : public tasks::RoutingTask::RoutingForwarder {
 public:
  MOCK_METHOD(void, forwardLocoChange,
              (const RR32Can::LocomotiveData& loco, const bool velocityChange,
               const bool directionChange,
               const RR32Can::LocomotiveData::FunctionBits_t functionChanges),
              (override));
  MOCK_METHOD(void, forward, (const RR32Can::Identifier rr32id, const RR32Can::Data& rr32data),
              (override));
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __MOCKS__ROUTINGFORWARDERMOCK_H__
