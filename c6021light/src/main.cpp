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

hal::CanTxCbk canTxCbk;

// Dummy variable that allows the toolchain to compile static variables that have destructors.
void* __dso_handle;

// ******** Code ********
extern "C" {
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
  hal::beginI2C(dataModel.kMyAddr, routingTask);
  hal::beginCan(routingTask);

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
  callbacks.tx = &canTxCbk;
  callbacks.system = &accessoryCbk;
  callbacks.engine = &routingTask.getCANEngineDB();
  RR32Can::RR32Can.begin(RR32CanUUID, callbacks);

  printf("Ready!\n");
  ConsoleManager::begin(&dataModel);
}

void setupOsTasks() {
  routingTask.Create("RoutingTask", configMAX_PRIORITIES - 1);
  consoleTask.Create("ConsoleTask",
                     0);  // Lowest prio as this task will always run.
}

// Main function for non-arduino
int main(void) {
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

namespace ConsoleManager {

int run_ln_slot_server_dump(int argc, const char* const* argv, int argcMatched) {
  static constexpr const char* appName{"LnSlotServerDump"};
  if (!checkNumArgs(argc - argcMatched, 0, 0, appName)) {
    display_help(argc, argv);
    return -2;
  }

  routingTask.getLnSlotServer().dump();

  return 0;
}

int run_app_download_enginedb_can(int, const char* const*, int) {
  routingTask.getCANEngineDB().fetchEngineDB();
  return 0;
}

int run_app_dump_enginedb_can(int, const char* const*, int) {
  routingTask.getCANEngineDB().dump();
  return 0;
}

}  // namespace ConsoleManager