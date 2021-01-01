// KeyboardDemoSketch
// Demonstrates the i2c communication with a Marklin Keyboard.
// Sends responses to a Keyboard and dumps the received command to the serial console.

// Note that this sketch is not robust against other messages. Recieving engine control messages
// Will likely require a reboot of the uC.

extern "C" {
#include <stdio.h>
#include <string.h>
}

#include <hal/LibOpencm3Hal.h>
#include "hal/stm32I2C.h"
#include "hal/stm32can.h"
#include "hal/stm32eepromEmulation.h"
#include "hal/stm32usart.h"

using Hal_t = hal::LibOpencm3Hal;

#include "ConsoleManager.h"
#include "DataModel.h"

extern "C" {
#include "microrl.h"
}

#include "RR32Can/StlAdapter.h"

#include "StationCbk.h"

#include "tasks/ConsoleTask/ConsoleTask.h"
#include "tasks/RoutingTask/RoutingTask.h"

#include "RR32Can/RR32Can.h"
#include "RR32Can_config.h"

#include <FreeRTOS.h>
#include "OsQueue.h"
#include "OsTask.h"

// ******** Variables and Constans********
Hal_t halImpl;
AccessoryCbk accessoryCbk;

DataModel dataModel;
freertossupport::StaticOsTask<tasks::RoutingTask::RoutingTask,
                              tasks::RoutingTask::RoutingTask::kStackSize>
    routingTask;
freertossupport::StaticOsTask<tasks::ConsoleTask::ConsoleTask,
                              tasks::ConsoleTask::ConsoleTask::kStackSize>
    consoleTask;

// ******** Code ********
extern "C" {
void vApplicationStackOverflowHook(xTaskHandle pxTask __attribute((unused)),
                                   portCHAR* pcTaskName __attribute((unused))) {
  for (;;) {
    __asm("bkpt 2");  // Loop forever here..
  }
}

void hard_fault_handler(void) {
  uint32_t cfsr = *(uint32_t*)0xE000ED28;
  uint16_t ufsr = *(uint16_t*)0xE000ED2A;
  uint8_t bfsr = *(uint8_t*)0xE000ED29;
  uint8_t mmfsr = *(uint8_t*)0xE000ED28;

  uint32_t hfsr = *(uint32_t*)0xE000ED2C;
  __asm("bkpt 1");
  (void)cfsr;
  (void)ufsr;
  (void)bfsr;
  (void)mmfsr;
  (void)hfsr;
}

void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer,
                                   StackType_t** ppxIdleTaskStackBuffer,
                                   uint32_t* pulIdleTaskStackSize) {
  static StackType_t idleTaskStack[configMINIMAL_STACK_SIZE];
  static StaticTask_t idleTaskTcb;
  *ppxIdleTaskTCBBuffer = &idleTaskTcb;
  *ppxIdleTaskStackBuffer = idleTaskStack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

}  // extern "C"

void setup() {
  // Setup I2C & CAN
  halImpl.begin();
  hal::beginSerial();
  hal::beginEE();
  hal::beginI2C(dataModel.myAddr, routingTask.getHandle());
  hal::beginCan(routingTask.getHandle());

  // Setup Serial
  printf("Connect6021Light Initializing...\n");

  // Load Configuration
  printf("Reading Configuration.\n");
  dataModel = hal::LoadConfig();
  routingTask.begin(dataModel);

  // Tie callbacks together
  printf("Setting up callbacks.\n");
  accessoryCbk.begin(halImpl);

  RR32Can::Station::CallbackStruct callbacks;
  callbacks.tx = &routingTask.canTxCbk_;
  callbacks.system = &accessoryCbk;
  RR32Can::RR32Can.begin(RR32CanUUID, callbacks);

  printf("Ready!\n");
  ConsoleManager::begin(&dataModel);
}

void setupOsResources() {
  constexpr static const uint8_t canqueuesize = 10;

  static freertossupport::StaticOsQueue<hal::CanQueueType::QueueElement, canqueuesize> canrxqbuffer;
  hal::canrxq = canrxqbuffer;

  constexpr static const uint8_t i2cqueuesize = canqueuesize;

  static freertossupport::StaticOsQueue<hal::I2CQueueType::QueueElement, i2cqueuesize> i2crxqBuffer;
  hal::i2cRxQueue = i2crxqBuffer;

  static freertossupport::StaticOsQueue<hal::I2CQueueType::QueueElement, i2cqueuesize> i2ctxqBuffer;
  hal::i2cTxQueue = i2ctxqBuffer;
}

void setupOsTasks() {
  routingTask.Create(routingTask, "RoutingTask", configMAX_PRIORITIES - 1);
  consoleTask.Create(consoleTask, "ConsoleTask",
                     0);  // Lowest prio as this task will always run.
}

// Main function for non-arduino
int main(void) {
  setupOsResources();
  setupOsTasks();

  setup();

  vTaskStartScheduler();

  while (1) {
    ;
  }
  return 0;
}

extern "C" void notifyLnByteReceived() {
  BaseType_t HigherPriorityTaskWoken = pdFALSE;
  routingTask.notifyFromISR(HigherPriorityTaskWoken);
  if (HigherPriorityTaskWoken == pdTRUE) {
    taskYIELD();
  }
}
