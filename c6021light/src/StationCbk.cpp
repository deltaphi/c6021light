#include "StationCbk.h"

#include "LocoNet.h"

#include "tasks/RoutingTask/RoutingTask.h"

void AccessoryCbk::begin(hal::LibOpencm3Hal& hal) { this->hal_ = &hal; }

void AccessoryCbk::setSystemState(bool onOff, bool response) {
  // When the system is stopped, turn on the LED.
  hal_->led(!onOff);
  if (!response) {
    LocoNet.reportPower(onOff);
  }
}
