#include <atomic>
#include <cstring>

#include "hal/stm32I2C.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#include "AtomicRingBuffer/ObjectRingBuffer.h"
#include "OsQueue.h"

#include "DataModel.h"

namespace hal {

namespace {
struct I2CBuf {
  constexpr const static uint8_t kMsgBytesLength = 3;
  uint8_t msgBytes[MarklinI2C::kMessageMaxBytes];
};

struct I2CTxBuf : public I2CBuf {
  uint_fast8_t bytesProcessed;
  std::atomic_bool bufferOccupied;
};

constexpr static const uint8_t kI2CQueueSize = 10;

using I2CQueueType = freertossupport::OsQueue<hal::I2CBuf>;
using QueueType = AtomicRingBuffer::ObjectRingBuffer<I2CMessage_t, kI2CQueueSize>;

static freertossupport::StaticOsQueue<hal::I2CQueueType::QueueElement, kI2CQueueSize> i2ctxqBuffer;

static uint8_t slaveAddress;

I2CTxBuf i2cRxBuf;
I2CTxBuf i2cTxBuf;

QueueType i2cRxQueue;
I2CQueueType i2cTxQueue = i2ctxqBuffer;

freertossupport::OsTask taskToNotify;

/**
 * \brief Checks whether the buffer contains a valid message.
 *
 * \return true if the message is valid, false otherwise.
 */
bool messageValid(const I2CTxBuf& buffer);

/**
 * \brief A buffer is occupied if the transmission ISR has access to it.
 *
 * This means: A transmission is...
 * * ongoing,
 * * pending,
 * * About to be done.
 */
bool bufferOccupied(const I2CTxBuf& buffer) {
  return buffer.bufferOccupied.load(std::memory_order_acquire);
}
/// A buffer is free if it is not occupied.
bool bufferFree(const I2CTxBuf& buffer) { return !bufferOccupied(buffer); }

void claimBuffer(I2CTxBuf& buffer) {
  i2cTxBuf.bytesProcessed = 0;
  buffer.bufferOccupied.store(true, std::memory_order_release);
}

void releaseBuffer(I2CTxBuf& buffer) {
  i2cTxBuf.bytesProcessed = 0;
  buffer.bufferOccupied.store(false, std::memory_order_release);
}

bool i2cBusy() { return (I2C_SR2(I2C1) & I2C_SR2_BUSY) != 0; }

/**
 * Take a message from the Queue and start transmitting.
 *
 * Function from Task
 */
void startTx();

/**
 * Take a message from the Queue and start transmitting.
 *
 * Function from ISR
 */
void startTxFromISR();

/**
 * Start a message from the filled TX buffer.
 */
void resumeTx();

/**
 * Same as resumeTx(), but ignores whether the bus is busy.
 */
void resumeTxForce();

/**
 * Abort a transmission and loose the message that may be transmitted.
 */
void finishTx();

/**
 * Send the message from the RX buffer for forwarding.
 */
void forwardRx(bool fromISR);

}  // namespace

// ***************************
// Implementation
// ***************************

void beginI2C(uint8_t newSlaveAddress, freertossupport::OsTask routingTask) {
  i2c_peripheral_disable(I2C1);
  i2c_reset(I2C1);

  slaveAddress = newSlaveAddress;
  taskToNotify = routingTask;

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
  releaseBuffer(i2cRxBuf);
  releaseBuffer(i2cTxBuf);
  nvic_enable_irq(NVIC_I2C1_EV_IRQ);
  nvic_enable_irq(NVIC_I2C1_ER_IRQ);
  nvic_set_priority(NVIC_I2C1_EV_IRQ, configMAX_SYSCALL_INTERRUPT_PRIORITY + 64);
  nvic_set_priority(NVIC_I2C1_ER_IRQ, configMAX_SYSCALL_INTERRUPT_PRIORITY + 64);
  i2c_enable_interrupt(I2C1, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN);

  i2c_peripheral_enable(I2C1);
  i2c_enable_ack(I2C1);
}

OptionalI2CMessage getI2CMessage() {
  const auto receiveResult = i2cRxQueue.peek();
  OptionalI2CMessage result;
  result.messageValid = receiveResult.ptr != nullptr;
  result.msg = *receiveResult.ptr;
  i2cRxQueue.consume(receiveResult);
  return result;
}

void sendI2CMessage(const I2CMessage_t& msg) {
  hal::I2CBuf buf;
  buf.msgBytes[0] = msg.destination_ >> 1;
  buf.msgBytes[1] = msg.data_;
  hal::i2cTxQueue.Send(buf, 0);  // TODO: Check the result.
  startTx();
}

namespace {
bool messageValid(const I2CTxBuf& buffer) {
  // Buffer occupied
  // its 3 bytes long.
  return bufferOccupied(buffer) && buffer.bytesProcessed == MarklinI2C::kAccessoryMessageLength;
}

void doTriggerTx(const I2CQueueType::ReceiveResult& receiveResult) {
  if (receiveResult.errorCode == pdPASS) {
    claimBuffer(i2cTxBuf);
    memcpy(i2cTxBuf.msgBytes, receiveResult.element.msgBytes, sizeof(I2CBuf::msgBytes));
    resumeTx();
  }
}

void startTx() {
  if (bufferFree(i2cTxBuf)) {
    // Non-Blocking receive
    I2CQueueType::ReceiveResult receiveResult = i2cTxQueue.Receive(0);
    doTriggerTx(receiveResult);
  }
}

void startTxFromISR() {
  if (bufferFree(i2cTxBuf)) {
    I2CQueueType::ReceiveResultISR receiveResult = i2cTxQueue.ReceiveFromISR();
    doTriggerTx(receiveResult);
    if (receiveResult.higherPriorityTaskWoken == pdTRUE) {
      taskYIELD();
    }
  } else {
    resumeTx();
  }
}

void resumeTx() {
  if (bufferOccupied(i2cTxBuf) && !i2cBusy()) {
    // If bus is idle, send a start condition.
    i2c_send_start(I2C1);
  }
}

void resumeTxForce() {
  if (bufferOccupied(i2cTxBuf)) {
    // If there is data, send a start condition.
    i2c_send_start(I2C1);
  }
}

void forwardRx(bool fromISR) {
  // Forecefully abort the current transaction
  i2c_peripheral_enable(I2C1);

  // If there is a complete message in the buffer, consider it received.
  if (messageValid(i2cRxBuf)) {
    auto memory = i2cRxQueue.allocate();
    if (memory.ptr != nullptr) {
      memory.ptr->destination_ = DataModel::kMyAddr;
      memory.ptr->source_ = i2cRxBuf.msgBytes[0];
      memory.ptr->data_ = i2cRxBuf.msgBytes[1];
      i2cRxQueue.publish(memory);
    } else {
      // TODO: Queue full. Handle somehow.
      __asm("bkpt 4");
    }

    releaseBuffer(i2cRxBuf);

    if (fromISR) {
      BaseType_t notifyWokeThread;
      taskToNotify.notifyFromISR(notifyWokeThread);

      if (notifyWokeThread == pdTRUE) {
        taskYIELD();
      }
    } else {
      taskToNotify.notify();
    }
    // See if there is something to be sent.
    startTxFromISR();
  }
}

void finishTx() {
  releaseBuffer(i2cTxBuf);
  i2c_send_stop(I2C1);

  // See if there is something to be sent.
  startTxFromISR();
}

}  // namespace

// i2c1 event ISR
// Code based on
// https://amitesh-singh.github.io/stm32/2018/01/07/making-i2c-slave-using-stm32f103.html
extern "C" void i2c1_ev_isr(void) {
  // ISR appears to be called once per I2C byte received
  // gpio_toggle(GPIOA, GPIO5);
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
      claimBuffer(i2cRxBuf);
      i2cRxBuf.bytesProcessed = 1;
    }
  }
  // Receive buffer not empty
  else if (sr1 & I2C_SR1_RxNE) {
    // Reference Manual: EV2
    if (bufferOccupied(i2cRxBuf)) {
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
        finishTx();
        break;
    }
  }
  // done by master by sending STOP
  // this event happens when slave is in Recv mode at the end of communication
  else if (sr1 & I2C_SR1_STOPF) {
    // Reference Manual: EV3
    forwardRx(true);
  }
  // this event happens when slave is in transmit mode at the end of communication
  else if (sr1 & I2C_SR1_AF) {
    //(void) I2C_SR1(I2C1);
    I2C_SR1(I2C1) &= ~(I2C_SR1_AF);
  }
}

// Error condition interrupt for I2C1
extern "C" void i2c1_er_isr(void) {
  // check which error bit was set
  uint32_t sr1 = I2C_SR1(I2C1);
  uint32_t sr2 = I2C_SR2(I2C1);

  // gpio_toggle(GPIOA, GPIO4);

  if (sr1 & I2C_SR1_AF) {
    // Acknowledge Failure (AF)
    // Reset AF bit
    I2C_SR1(I2C1) &= ~I2C_SR1_AF;
    // Abort the current transmission. Assume that noone is available on the bus.
    finishTx();
    //__asm("bkpt 6");

  } else if (sr1 & I2C_SR1_ARLO) {
    // Arbitration Lost
    // Interface automatically goes to slave mode.
    // End of reception will trigger another attempt at starting.
    //__asm("bkpt 6");

  } else if (sr1 & I2C_SR1_BERR) {
    if (sr2 & I2C_SR2_MSL) {
      // In Master mode: software must clean up
      //__asm("bkpt 6");
      // In out usecase, this is caused by a misbehaving Memory device.
      // Restart the transmission.
      resumeTxForce();

    } else {
      // In Slave mode: auto-release
      //__asm("bkpt 6");
      // If a valid message was received, this is caused by a misbehaving Memory devide.
      forwardRx(true);
    }

    // Reset the error flag as the error was handled.
    I2C_SR1(I2C1) &= ~I2C_SR1_BERR;

  } else if (sr1 & I2C_SR1_TxE) {
    __asm("bkpt 6");

  } else if (sr1 & I2C_SR1_RxNE) {
    __asm("bkpt 6");

  } else {
    __asm("bkpt 6");
  }
}

}  // namespace hal
