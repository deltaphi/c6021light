#include "hal/stm32I2C.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

namespace hal {

static uint8_t slaveAddress;

I2CBuf i2cRxBuf;
I2CBuf i2cTxBuf;

void beginI2C(uint8_t newSlaveAddress) {
  i2c_peripheral_disable(I2C1);
  i2c_reset(I2C1);

  slaveAddress = newSlaveAddress;

  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
                GPIO_I2C1_SCL | GPIO_I2C1_SDA);
  gpio_set(GPIOB, GPIO_I2C1_SCL | GPIO_I2C1_SDA);

  // Basic I2C configuration
  // i2c_set_speed(I2C1, i2c_speed_sm_100k, I2C_CR2_FREQ_36MHZ);
  i2c_set_standard_mode(I2C1);
  i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);
  i2c_set_trise(I2C1, 36);
  i2c_set_dutycycle(I2C1, I2C_CCR_DUTY_DIV2);
  i2c_set_ccr(I2C1, 180);

  i2c_set_own_7bit_slave_address(I2C1, slaveAddress);

  // Set I2C IRQ to support slave mode
  i2cTxBuf.bytesProcessed = 0;
  i2cRxBuf.bytesProcessed = 0;
  i2cTxBuf.msgValid.store(false, std::memory_order_release);
  i2cRxBuf.msgValid.store(false, std::memory_order_release);
  nvic_enable_irq(NVIC_I2C1_EV_IRQ);
  i2c_enable_interrupt(I2C1, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);

  i2c_peripheral_enable(I2C1);
  i2c_enable_ack(I2C1);
}

void triggerI2cTx() {
  if (i2cTxMsgAvailable()) {
    i2c_send_start(I2C1);
  }
}

// i2c1 event ISR
// Code based on
// https://amitesh-singh.github.io/stm32/2018/01/07/making-i2c-slave-using-stm32f103.html
extern "C" void i2c1_ev_isr(void) {
  // ISR appears to be called once per I2C byte received

  uint32_t sr1, sr2;

  sr1 = I2C_SR1(I2C1);

  if (sr1 & I2C_SR1_SB) {
    // Refrence Manual: EV5 (Master)
    i2cTxBuf.bytesProcessed = 1;
    i2c_send_7bit_address(I2C1, i2cTxBuf.msgBytes[0], I2C_WRITE);
  } else if (sr1 & I2C_SR1_ADDR) {
    // Address matched (Slave)
    // Refrence Manual: EV6 (Master)/EV1 (Slave)

    // Clear the ADDR sequence by reading SR2.
    sr2 = I2C_SR2(I2C1);

    if (!(sr2 & I2C_SR2_MSL)) {
      // Reference Manual: EV1
      if (!i2cRxBuf.msgValid.load(std::memory_order_acquire)) {
        i2cRxBuf.bytesProcessed = 1;
      }
    }
  }
  // Receive buffer not empty
  else if (sr1 & I2C_SR1_RxNE) {
    // Reference Manual: EV2
    if (!i2cRxBuf.msgValid.load(std::memory_order_acquire)) {
      if (i2cRxBuf.bytesProcessed < 3) {
        i2cRxBuf.msgBytes[i2cRxBuf.bytesProcessed - 1] = i2c_get_data(I2C1);
        ++i2cRxBuf.bytesProcessed;
      }
    }
  }
  // Transmit buffer empty & Data byte transfer not finished
  else if ((sr1 & I2C_SR1_TxE) && !(sr1 & I2C_SR1_BTF)) {
    // EV8, 8_1
    switch (i2cTxBuf.bytesProcessed) {
      case 1:
        i2c_send_data(I2C1, slaveAddress << 1);
        ++i2cTxBuf.bytesProcessed;
        break;
      case 2:
        i2c_send_data(I2C1, i2cTxBuf.msgBytes[1]);
        ++i2cTxBuf.bytesProcessed;
        break;
      default:
        // EV 8_2
        i2cTxBuf.msgValid.store(false, std::memory_order_release);
        i2c_send_stop(I2C1);
        break;
    }
  }
  // done by master by sending STOP
  // this event happens when slave is in Recv mode at the end of communication
  else if (sr1 & I2C_SR1_STOPF) {
    // Reference Manual: EV3
    i2c_peripheral_enable(I2C1);
    if (i2cRxBuf.bytesProcessed == 3) {
      i2cRxBuf.msgValid.store(true, std::memory_order_release);
    }
  }
  // this event happens when slave is in transmit mode at the end of communication
  else if (sr1 & I2C_SR1_AF) {
    //(void) I2C_SR1(I2C1);
    I2C_SR1(I2C1) &= ~(I2C_SR1_AF);
  }
}

}  // namespace hal
