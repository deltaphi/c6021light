#include "hal/LibOpencm3Hal.h"

#include <limits>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

#include <errno.h>
#include <unistd.h>

#include "RR32Can/RR32Can.h"

#include "ee.h"
#include "eeConfig.h"

#include <LocoNet.h>

extern "C" {
/* _write code taken from example at
 * https://github.com/libopencm3/libopencm3-examples/blob/master/examples/stm32/l1/stm32l-discovery/button-irq-printf-lowpower/main.c
 */
int _write(int file, char* ptr, int len) {
  int i;

  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
    int bytesLeft = len;
    while (bytesLeft > 0) {
      AtomicRingBuffer::size_type bytesWritten =
          hal::LibOpencm3Hal::instance_->SerialWrite(ptr, bytesLeft);
      bytesLeft -= bytesWritten;
    }
    return i;
  }
  errno = EIO;
  return -1;
}

void dma1_channel4_isr(void) { hal::LibOpencm3Hal::instance_->irqSerialTxDMA(); }
}

namespace hal {

LibOpencm3Hal* LibOpencm3Hal::instance_;

uint8_t LibOpencm3Hal::SerialWrite(char* ptr, AtomicRingBuffer::size_type len) {
  /*
    for (i = 0; i < len; i++) {
      if (ptr[i] == '\n') {
          usart_send_blocking(USART1, '\r');
        }
        usart_send_blocking(USART1, ptr[i]);
    }
    */

  // Copy Data to buffer
  AtomicRingBuffer::pointer_type mem;
  AtomicRingBuffer::size_type numBytes = serialBuffer_.allocate(mem, len, true);
  if (numBytes > 0) {
    strncpy(reinterpret_cast<char*>(mem), ptr, numBytes);
    for (std::size_t i = 0; i < numBytes; ++i) {
      if (mem[i] == '\n') {
        mem[i] = '\r';
      }
    }
    serialBuffer_.publish(mem, numBytes);
    startSerialTx();
  }

  // Start DMA

  // Return how many bytes were sent off
  return numBytes;
}

void LibOpencm3Hal::startSerialTx() {
  if (serialBuffer_.size() > 0) {
    // There is data to be transferred.
    bool dmaBusy = false;
    if (serialDmaBusy_.compare_exchange_strong(dmaBusy, true, std::memory_order_acq_rel)) {
      serialNumBytes_ = serialBuffer_.peek(
          serialMem_, std::numeric_limits<AtomicRingBuffer::size_type>::max(), true);

      dma_channel_reset(DMA1, DMA_CHANNEL4);

      dma_set_memory_size(DMA1, DMA_CHANNEL4, 1);

      dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t)&USART1_DR);
      dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t)serialMem_);
      uint32_t dmaNumBytes = serialNumBytes_;  /// sizeof(uint32_t);
      // serialNumBytes_ = dmaNumBytes * sizeof(uint32_t);
      dma_set_number_of_data(DMA1, DMA_CHANNEL4, dmaNumBytes);

      dma_set_read_from_memory(DMA1, DMA_CHANNEL4);
      dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);

      // dma_enable_peripheral_increment_mode(DMA1, DMA_CHANNEL4);

      dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_8BIT);
      dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_8BIT);
      dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_VERY_HIGH);

      dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

      usart_enable_tx_dma(USART1);

      dma_enable_channel(DMA1, DMA_CHANNEL4);

      //USART_SR(USART1) &= ~USART_SR_TC;
    }
  }
}

void LibOpencm3Hal::irqSerialTxDMA() {
  if ((DMA1_ISR & DMA_ISR_TCIF4) != 0) {
    DMA1_IFCR |= DMA_IFCR_CTCIF4;

    serialBuffer_.consume(serialMem_, serialNumBytes_);
    serialDmaBusy_.store(false, std::memory_order_release);
  }

  dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

  usart_disable_tx_dma(USART1);

  dma_disable_channel(DMA1, DMA_CHANNEL4);

  // Check if there is data remaining.
  startSerialTx();
}

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

  // Enable the DMA clock
  rcc_periph_clock_enable(RCC_DMA1);
}

void LibOpencm3Hal::beginGpio() {
  gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO13);
  gpio_set(GPIOC, GPIO13);  // Turn the LED off.

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO0);  // Extra LED
  gpio_set(GPIOA, GPIO0);  // Set Idle High (TODO: Correct?)

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                GPIO4 | GPIO5 | GPIO6);  // Extra LED
  gpio_set(GPIOA, GPIO4 | GPIO5 | GPIO6);
}

void LibOpencm3Hal::beginLocoNet() { LocoNet.init(PinNames::PB15); }

void LibOpencm3Hal::beginSerial() {
  serialDmaBusy_ = false;
  serialBuffer_.init(bufferMemory_, bufferSize_);
  instance_ = this;

  usart_disable(USART1);

  // Enable the USART TX Pin in the GPIO controller
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

  // Set Serial speed
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

  // Enable Serial TX
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_enable(USART1);

  // Setup serial DMA IRQ
  nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, configMAX_SYSCALL_INTERRUPT_PRIORITY + 64);
  nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);
}

void LibOpencm3Hal::led(bool on) {
  if (on) {
    gpio_clear(GPIOC, GPIO13);
  } else {
    gpio_set(GPIOC, GPIO13);
  }
}

void LibOpencm3Hal::toggleLed() { gpio_toggle(GPIOC, GPIO13); }

void LibOpencm3Hal::loopSerial() {
  // Check if a byte has been received.
  if ((USART_SR(USART1) & USART_SR_RXNE) != 0) {
    uint16_t character = usart_recv(USART1);
    microrl_insert_char(this->console_->getMicroRl(), character);
  }
}

void LibOpencm3Hal::beginEE() { ee_init(); }

DataModel LibOpencm3Hal::LoadConfig() {
  DataModel model;
  ee_read(DataAddresses::accessoryRailProtocol, sizeof(DataModel::accessoryRailProtocol),
          reinterpret_cast<uint8_t*>(&model.accessoryRailProtocol));
  return model;
}

void LibOpencm3Hal::SaveConfig(const DataModel& model) {
  ee_writeToRam(DataAddresses::accessoryRailProtocol, sizeof(DataModel::accessoryRailProtocol),
                reinterpret_cast<uint8_t*>(&const_cast<DataModel&>(model).accessoryRailProtocol));
  ee_commit();
}

extern "C" uint8_t HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef* addr, uint32_t* error) {
  if (addr->NbPages == 1) {
    flash_erase_page(addr->PageAddress);
    *error = flash_get_status_flags();
    if (*error != FLASH_SR_EOP) {
      return HAL_NOK;
    } else {
      *error = 0xFFFFFFFF;  // Expected value by caller of OK case.
      return HAL_OK;
    }
  } else {
    printf("HAL_FLASHEx_Erase: requested erase not supported.\n");
    return HAL_NOK;
  }
}

extern "C" uint8_t HAL_FLASH_Program(uint8_t flashProgramType, uint32_t addr, uint64_t data) {
  if (flashProgramType == FLASH_TYPEPROGRAM_HALFWORD) {
    flash_program_half_word(addr, data);
    uint32_t flashStatus = flash_get_status_flags();
    if (flashStatus != FLASH_SR_EOP) {
      return HAL_NOK;
    } else {
      return HAL_OK;
    }
  } else {
    printf("HAL_FLASH_Program: flashProgramType not supported.\n");
    return HAL_NOK;
  }
}

}  // namespace hal