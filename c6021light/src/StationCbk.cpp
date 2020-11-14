#include "StationCbk.h"

#include "tasks/RoutingTask/RoutingTask.h"

void AccessoryCbk::begin(hal::HalBase& hal) { this->hal_ = &hal; }

void AccessoryCbk::setSystemState(bool onOff) {
  // When the system is stopped, turn on the LED.
  hal_->led(!onOff);
}
