#include "hal/LibOpencm3Hal.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/usart.h>

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

#include <errno.h>
#include <unistd.h>

#include "RR32Can/RR32Can.h"

#include "ee.h"
#include "eeConfig.h"

#include "FreeRTOS.h"

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

namespace hal {

freertossupport::OsQueue<LibOpencm3Hal::CanMsg> LibOpencm3Hal::canrxq;
TaskHandle_t LibOpencm3Hal::taskToNotify;

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

void LibOpencm3Hal::beginSerial() {
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

extern "C" {
void usb_lp_can_rx0_isr(void) {
  BaseType_t anyTaskWoken = pdFALSE;
  for (uint32_t messageCount = CAN_RF0R(CAN1) & 3; messageCount > 0; --messageCount) {
    LibOpencm3Hal::CanMsg canMsg;

    bool ext;
    bool rtr;
    uint8_t fmi;
    uint16_t timestamp;

    can_receive(CAN1, 0, true, &canMsg.id, &ext, &rtr, &fmi, &canMsg.data.dlc, canMsg.data.data,
                &timestamp);
    LibOpencm3Hal::CanQueueType::SendResultISR sendResult =
        LibOpencm3Hal::canrxq.SendFromISR(canMsg);
    if (sendResult.errorCode != pdTRUE) {
      // TODO: Handle Queue full by notifying the user.
      __asm("bkpt 4");
    } else {
      if (sendResult.higherPriorityTaskWoken == pdTRUE) {
        anyTaskWoken = pdTRUE;
      }
      break;
    }
  }
  BaseType_t taskWoken;
  BaseType_t notifyResult =
      xTaskNotifyFromISR(LibOpencm3Hal::taskToNotify, 1, eSetValueWithoutOverwrite, &taskWoken);

  if (anyTaskWoken == pdTRUE || taskWoken == pdTRUE) {
    taskYIELD();
  }
}
}

void LibOpencm3Hal::loopCan() {
  constexpr const TickType_t ticksToWait = 0;
  for (CanQueueType::ReceiveResult receiveResult = canrxq.Receive(ticksToWait);
       receiveResult.errorCode == pdTRUE; receiveResult = canrxq.Receive(ticksToWait)) {
    RR32Can::Identifier rr32id = RR32Can::Identifier::GetIdentifier(receiveResult.element.id);
    RR32Can::RR32Can.HandlePacket(rr32id, receiveResult.element.data);
  }
}

void LibOpencm3Hal::SendPacket(RR32Can::Identifier const& id, RR32Can::Data const& data) {
  uint32_t packetId = id.makeIdentifier();
  can_transmit(CAN1, packetId, true, false, data.dlc, const_cast<uint8_t*>(data.data));
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