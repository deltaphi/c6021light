#include "hal/stm32usart.h"

#include <errno.h>
#include <unistd.h>

#include <limits>
#include <mutex>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include "AtomicRingBuffer/AtomicRingBuffer.h"
#include "AtomicRingBuffer/StringCopyHelper.h"

#include "FreeRTOSConfig.h"

#include "OsMutex.h"

namespace hal {

// Definition of Module-private functions

void startSerialTx();
void irqSerialTxDMA();

uint8_t SerialWrite(const char* ptr, AtomicRingBuffer::AtomicRingBuffer::size_type len,
                    bool doReplace);

freertossupport::OsMutexStatic serialWriteMutex_;

// Definition of Module-private data

constexpr static const AtomicRingBuffer::AtomicRingBuffer::size_type bufferSize_ = 1024;
AtomicRingBuffer::AtomicRingBuffer serialBuffer_;
uint8_t bufferMemory_[bufferSize_];

std::atomic_bool serialDmaBusy_;

AtomicRingBuffer::AtomicRingBuffer::MemoryRange serialDmaMemory_;

// Actual Code

void beginSerial() {
  serialWriteMutex_.Create();

  serialDmaBusy_ = false;
  serialBuffer_.init(bufferMemory_, bufferSize_);

  usart_disable(USART1);

  // Enable the USART TX Pin in the GPIO controller
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
  gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

  // Set Serial speed
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

  // Enable Serial TX
  usart_enable(USART1);

  // Setup serial DMA IRQ
  nvic_set_priority(NVIC_DMA1_CHANNEL4_IRQ, configMAX_SYSCALL_INTERRUPT_PRIORITY + 64);
  nvic_enable_irq(NVIC_DMA1_CHANNEL4_IRQ);
}

bool pollSerial(uint16_t& receivedChar) {
  // Check if a byte has been received.
  if ((USART_SR(USART1) & USART_SR_RXNE) != 0) {
    receivedChar = usart_recv(USART1);
    return true;
  } else {
    return false;
  }
}

extern "C" {
/* _write code derived and severely extended from example at
 * https://github.com/libopencm3/libopencm3-examples/blob/master/examples/stm32/l1/stm32l-discovery/button-irq-printf-lowpower/main.c
 */
int _write(int file, char* ptr, int len) {
  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
    int bytesLeft = len;
    while (bytesLeft > 0) {
      AtomicRingBuffer::AtomicRingBuffer::size_type bytesWritten =
          SerialWrite(ptr, bytesLeft, true);
      bytesLeft -= bytesWritten;
    }
    return len - bytesLeft;
  }
  errno = EIO;
  return -1;
}

void dma1_channel4_isr(void) {
  if ((DMA1_ISR & DMA_ISR_TCIF4) != 0) {
    DMA1_IFCR |= DMA_IFCR_CTCIF4;

    serialBuffer_.consume(serialDmaMemory_);
    serialDmaBusy_.store(false, std::memory_order_release);
  }

  dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

  usart_disable_tx_dma(USART1);

  dma_disable_channel(DMA1, DMA_CHANNEL4);

  // Check if there is data remaining.
  startSerialTx();
}
}  // extern "C"

uint8_t SerialWrite(const char* src, AtomicRingBuffer::AtomicRingBuffer::size_type srcLen,
                    bool doReplace) {
  using size_type = AtomicRingBuffer::AtomicRingBuffer::size_type;
  using MemoryRange = AtomicRingBuffer::AtomicRingBuffer::MemoryRange;

  // Constants used for memcpyCharReplace
  constexpr static char search = '\n';
  constexpr static const char* replace = "\r\n";
  constexpr static uint8_t replaceLen = strlen(replace);
  static_assert(replaceLen == 2, "strlen(replace) does not work as expected.");

  // Optimization for a common case: When the input string end in a newline, request enough
  // additional characters to fit at least one replace.
  size_type expectedDestLen = srcLen;
  if (src[srcLen - 1] == '\n') {
    expectedDestLen += replaceLen - 1;
  }

  size_type srcBytesConsumed = 0;

  while (srcBytesConsumed < srcLen) {
    const char* replacePtr = replace;
    {
      // pointer_type dest;

      size_type bytesConsumed = 0;

      {
        std::lock_guard<freertossupport::OsMutex> guard{serialWriteMutex_};
        MemoryRange dest = serialBuffer_.allocate(expectedDestLen, true);

        if (doReplace) {
          auto replaceResult = AtomicRingBuffer::memcpyCharReplace(
              reinterpret_cast<char*>(dest.ptr), &src[srcBytesConsumed], search, replacePtr,
              dest.len, srcLen);
          dest.len = reinterpret_cast<uint8_t*>(replaceResult.nextByte) - dest.ptr;
        } else {
          memcpy(dest.ptr, &src[srcBytesConsumed], dest.len);
          bytesConsumed = dest.len;
        }

        serialBuffer_.publish(dest);
      }
      startSerialTx();

      srcBytesConsumed += bytesConsumed;
      expectedDestLen -= bytesConsumed;
    }
    bool replaceIncomplete = replacePtr != replace;

    if (replaceIncomplete && doReplace) {
      // There were bytes left, try to send those *without* replacement.
      size_type remainingReplaceLen = replaceLen - (replacePtr - replace);
      SerialWrite(replacePtr, remainingReplaceLen, false);
    }
  }
  // Return how many bytes were sent off

  return srcBytesConsumed;
}

void startSerialTx() {
  if (serialBuffer_.size() > 0) {
    // There is data to be transferred.
    bool dmaBusy = false;
    if (serialDmaBusy_.compare_exchange_strong(dmaBusy, true, std::memory_order_acq_rel)) {
      serialDmaMemory_ = serialBuffer_.peek(
          std::numeric_limits<AtomicRingBuffer::AtomicRingBuffer::size_type>::max(), true);

      dma_channel_reset(DMA1, DMA_CHANNEL4);

      dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t)&USART1_DR);
      dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t)serialDmaMemory_.ptr);
      dma_set_number_of_data(DMA1, DMA_CHANNEL4, serialDmaMemory_.len);

      dma_set_read_from_memory(DMA1, DMA_CHANNEL4);
      dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);

      // dma_enable_peripheral_increment_mode(DMA1, DMA_CHANNEL4);

      dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_8BIT);
      dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_8BIT);
      dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_VERY_HIGH);

      dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);

      dma_enable_channel(DMA1, DMA_CHANNEL4);

      usart_enable_tx_dma(USART1);
    }
  }
}

}  // namespace hal
