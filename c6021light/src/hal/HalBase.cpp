#include "hal/HalBase.h"

namespace hal {

uint8_t HalBase::i2cLocalAddr;

MarklinI2C::Messages::AccessoryMsg HalBase::prepareI2cMessage() {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.source = i2cLocalAddr;
  return msg;
}

}  // namespace hal
