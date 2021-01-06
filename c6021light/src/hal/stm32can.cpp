#include "hal/stm32can.h"

#include <cstdio>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/gpio.h>

#include "AtomicRingBuffer/ObjectRingBuffer.h"

#include "OsTask.h"

#include "RR32Can/RR32Can.h"
#include "RR32Can/messages/Identifier.h"

namespace hal {
constexpr static const uint8_t canqueuesize = 10;

using CanQueueType = AtomicRingBuffer::ObjectRingBuffer<RR32Can::CanFrame, canqueuesize>;
static CanQueueType canRxQueue;

static freertossupport::OsTask taskToNotify;

void beginCan(freertossupport::OsTask task) {
  taskToNotify = task;
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
  nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
  nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, configMAX_SYSCALL_INTERRUPT_PRIORITY + 64);
  can_enable_irq(CAN1, CAN_IER_FMPIE0);

  if (can_init(CAN1, false, true, false, false, false, false, CAN_BTR_SJW_1TQ, CAN_BTR_TS1_13TQ,
               CAN_BTR_TS2_2TQ, 9, false, false)) {
    /* Die because we failed to initialize. */
    printf("CAN failed to initialize.\n");
    while (1) __asm__("nop");
  }

  /* CAN filter 0 init. */
  can_filter_id_mask_32bit_init(0,     /* Filter ID */
                                0,     /* CAN ID */
                                0,     /* CAN ID mask */
                                0,     /* FIFO assignment (here: FIFO0) */
                                true); /* Enable the filter. */
}

RR32Can::CanFrame* getCanMessage() { return canRxQueue.peek().ptr; }

std::size_t freeCanMessage(RR32Can::CanFrame* ptr) {
  return canRxQueue.consume(CanQueueType::MemoryRange{ptr, 1});
}

extern "C" {
void usb_lp_can_rx0_isr(void) {
  for (uint32_t messageCount = CAN_RF0R(CAN1) & 3; messageCount > 0; --messageCount) {
    auto canMsg = canRxQueue.allocate();

    if (canMsg.ptr != nullptr) {
      bool ext;
      bool rtr;
      uint8_t fmi;
      uint16_t timestamp;

      can_receive(CAN1, 0, true, &(canMsg.ptr->id.rawValue()), &ext, &rtr, &fmi,
                  &canMsg.ptr->data.dlc, canMsg.ptr->data.data, &timestamp);
      canRxQueue.publish(canMsg);
    } else {
      // TODO: Handle Queue full by notifying the user.
      __asm("bkpt 4");
    }
  }
  taskToNotify.notifyFromISRWithWake();
}
}

void CanTxCbk::SendPacket(const RR32Can::CanFrame& frame) {
  uint32_t packetId = frame.id.makeIdentifier();
  int txMailbox = -1;
  while (txMailbox == -1) {
    // Busy-wait until a mailbox becomes free.
    txMailbox = can_transmit(CAN1, packetId, true, false, frame.data.dlc,
                             const_cast<uint8_t*>(frame.data.data));
  }
}

}  // namespace hal
