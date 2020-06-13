#include "hal/LibOpencm3Hal.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/usart.h>

#include <stdio.h>

#include <errno.h>
#include <unistd.h>

#include "RR32Can/RR32Can.h"

/** Duration of an LSI-driven RTC tick in us.
 *  Manually calibrated for LSI, may vary for different boards.
 */
constexpr const uint32_t kRTCTickDuration = 49;

/// Prescaler value to achieve kRTCTickDuration when using LSI.
constexpr const uint32_t kRTC_LSI_Prescaler = 1;  // ~49us resolution.

// Forward declarations
void setup();
void loop();

// Main function for non-arduino
int main(void) {
  setup();
  while (1) {
    loop();
  }
  return 0;
}

extern "C" {
/* _write code taken from example at
 * https://github.com/libopencm3/libopencm3-examples/blob/master/examples/stm32/l1/stm32l-discovery/button-irq-printf-lowpower/main.c
 */
int _write(int file, char* ptr, int len) {
  int i;

  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
    for (i = 0; i < len; i++) {
      if (ptr[i] == '\n') {
        usart_send_blocking(USART1, '\r');
      }
      usart_send_blocking(USART1, ptr[i]);
    }
    return i;
  }
  errno = EIO;
  return -1;
}
}

unsigned long micros() {
  uint32_t rtcCounter = rtc_get_counter_val();
  return rtcCounter * kRTCTickDuration;
}

unsigned long seconds() { return micros() / 1000000; }

unsigned long millis() { return micros() / 1000; }

namespace hal {

LibOpencm3Hal::I2CBuf LibOpencm3Hal::i2cRxBuf;
LibOpencm3Hal::I2CBuf LibOpencm3Hal::i2cTxBuf;

void LibOpencm3Hal::beginClock() {
  // Enable the overall clock.
  rcc_clock_setup_in_hse_8mhz_out_72mhz();

  // Enable GPIO Pin Banks used for GPIO or alternate functions
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  // Enable Clock for alternate functions
  rcc_periph_clock_enable(RCC_AFIO);

  // Enable the UART clock
  rcc_periph_clock_enable(RCC_USART1);

  // Enable the I2C clock
  rcc_periph_clock_enable(RCC_I2C1);

  // Enable the CAN clock
  rcc_periph_clock_enable(RCC_CAN1);
}

void LibOpencm3Hal::beginGpio() {
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO13);
  gpio_set(GPIOC, GPIO13);  // Turn the LED off.
}

void LibOpencm3Hal::beginRtc() {
  rtc_awake_from_off(LSI);
  rtc_set_prescale_val(kRTC_LSI_Prescaler);
}

void LibOpencm3Hal::beginSerial() {
  usart_disable(USART1);

  // Enable the USART TX Pin in the GPIO controller
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

  // Set Serial speed
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

  // Enable Serial TX
  usart_set_mode(USART1, USART_MODE_TX);
  usart_enable(USART1);
}

void LibOpencm3Hal::beginI2c() {
  i2c_peripheral_disable(I2C1);
  i2c_reset(I2C1);

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

  i2c_set_own_7bit_slave_address(I2C1, this->i2cLocalAddr);

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

// i2c1 event ISR
// Code based on
// https://amitesh-singh.github.io/stm32/2018/01/07/making-i2c-slave-using-stm32f103.html
extern "C" void i2c1_ev_isr(void) { LibOpencm3Hal::i2cEvInt(); }

void LibOpencm3Hal::i2cEvInt(void) {
  // ISR appears to be called once per I2C byte received

  uint32_t sr1, sr2;

  sr1 = I2C_SR1(I2C1);

  if (sr1 & I2C_SR1_SB) {
    // Refrence Manual: EV5 (Master)
    i2cTxBuf.bytesProcessed = 1;
    i2c_send_7bit_address(I2C1, i2cTxBuf.msgBytes[0], I2C_WRITE);
  } else
      // Address matched (Slave)
      if (sr1 & I2C_SR1_ADDR) {
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
        i2c_send_data(I2C1, i2cLocalAddr << 1);
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

void LibOpencm3Hal::beginCan() {
  AFIO_MAPR |= AFIO_MAPR_CAN1_REMAP_PORTB;

  /* Configure CAN pin: RX (input pull-up) */
  gpio_set_mode(GPIO_BANK_CAN1_PB_RX, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_CAN1_PB_RX);
  gpio_set(GPIO_BANK_CAN1_PB_RX, GPIO_CAN1_PB_RX);

  /* Configure CAN pin: TX */
  gpio_set_mode(GPIO_BANK_CAN1_PB_TX, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                GPIO_CAN1_PB_TX);

  /* Reset CAN */
  can_reset(CAN1);

  /* default CAN setting 250 kBaud */

  if (can_init(CAN1, false, true, false, false, false, false, CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ,
               CAN_BTR_TS2_2TQ, 9, false, false)) {
    /* Die because we failed to initialize. */
    printf("CAN failed to initialize.\n");
    while (1) __asm__("nop");
  }

  /* CAN filter 0 init. */
  can_filter_id_mask_32bit_init(CAN1, 0, /* Filter ID */
                                0,       /* CAN ID */
                                0,       /* CAN ID mask */
                                0,       /* FIFO assignment (here: FIFO0) */
                                true);   /* Enable the filter. */
}

void LibOpencm3Hal::loopCan() {
  if (CAN_RF0R(CAN1) & CAN_RF0R_FMP0_MASK) {  // message available in FIFO 0
    uint32_t packetId;

    bool ext;
    bool rtr;
    uint32_t fmi;
    RR32Can::Data data;

    can_receive(CAN1, 0, true, &packetId, &ext, &rtr, &fmi, &data.dlc, data.data);

    RR32Can::Identifier rr32id = RR32Can::Identifier::GetIdentifier(packetId);
    RR32Can::RR32Can.HandlePacket(rr32id, data);
  }
}

void LibOpencm3Hal::SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg) {
  i2cTxBuf.bytesProcessed = 0;
  i2cTxBuf.msgBytes[0] = msg.destination;
  i2cTxBuf.msgBytes[1] = msg.data;
  i2cTxBuf.msgValid.store(true, std::memory_order_release);
  i2c_send_start(I2C1);
}

void LibOpencm3Hal::SendPacket(RR32Can::Identifier const& id, RR32Can::Data const& data) {
  uint32_t packetId = id.makeIdentifier();
  can_transmit(CAN1, packetId, true, false, data.dlc, const_cast<uint8_t*>(data.data));
}

MarklinI2C::Messages::AccessoryMsg LibOpencm3Hal::getI2cMessage() const {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.destination = i2cLocalAddr;
  msg.source = i2cRxBuf.msgBytes[0];
  msg.data = i2cRxBuf.msgBytes[1];
  i2cRxBuf.bytesProcessed = 0;
  i2cRxBuf.msgValid.store(false, std::memory_order_release);
  return msg;
}

}  // namespace hal