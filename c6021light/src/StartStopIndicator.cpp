#include "StartStopIndicator.h"

void StartStopIndicator::setSystemState(bool onOff, bool) {
  if (!onOff) {
    setState(State::START);
  } else {
    setState(State::STOP);
  }
}

void StartStopIndicator::setUnkown() { setState(State::UNKNOWN); }

void StartStopIndicator::setState(StartStopIndicator::State state) {
  state_ = state;
  switch (state) {
    case State::STOP:
      timer_.Stop();
      hal_->startStopLed.off();
      break;
    case State::START:
      timer_.Stop();
      hal_->startStopLed.on();
      break;
    case State::UNKNOWN:
      timer_.Start(kUnknownBlinkPeriod);
      break;
  }
}

void StartStopIndicator::TimerCallback(TimerHandle_t) {
  if (state_ == State::UNKNOWN) {
    hal_->startStopLed.toggle();
  }
}
