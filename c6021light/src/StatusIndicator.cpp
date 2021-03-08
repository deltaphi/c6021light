#include "StatusIndicator.h"

void StatusIndicator::setState(StatusIndicator::State state) {
  state_ = state;
  switch (state) {
    case State::OFF:
      timer_.Stop();
      hal_->statusLed.off();
      break;
    case State::IDLE:
      timer_.Stop();
      hal_->statusLed.on();
      break;
    case State::CANDB_DL:
      timer_.Start(kCandbDlBlinkPeriod);
      break;
    case State::ERROR:
      timer_.Start(kErrorBlinkPeriod);
      break;
  }
}

void StatusIndicator::TimerCallback(TimerHandle_t) {
  if (state_ == State::ERROR || state_ == State::CANDB_DL) {
    hal_->statusLed.toggle();
  }
}
