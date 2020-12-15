#include "StationCbk.h"

#include "LocoNet.h"

#include "tasks/RoutingTask/RoutingTask.h"

void AccessoryCbk::begin(hal::LibOpencm3Hal& hal) { this->hal_ = &hal; }

void AccessoryCbk::setSystemState(bool onOff, bool) {
  // When the system is stopped, turn on the LED.
  hal_->led(!onOff);
}
