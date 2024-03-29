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

#include <FreeRTOS.h>
#include "OsTask.h"
#include "OsTimer.h"
#include "timers.h"

#include "RR32Can/RR32Can.h"
#include "RR32Can/StlAdapter.h"
#include "RR32Can_config.h"

using Hal_t = hal::LibOpencm3Hal;

extern "C" {
#include "microrl.h"
}

#include "ConsoleManager.h"
#include "DataModel.h"
#include "StartStopIndicator.h"
#include "StatusIndicator.h"
#include "tasks/ConsoleTask/ConsoleTask.h"
#include "tasks/RoutingTask/RoutingTask.h"

// ******** Variables and Constans********
Hal_t halImpl;

DataModel dataModel;
freertossupport::StaticOsTask<tasks::RoutingTask::RoutingTask,
                              tasks::RoutingTask::RoutingTask::kStackSize>
    routingTask;
freertossupport::StaticOsTask<tasks::ConsoleTask::ConsoleTask,
                              tasks::ConsoleTask::ConsoleTask::kStackSize>
    consoleTask;

freertossupport::StaticOsTimer stopGoTimer;
freertossupport::StaticOsTimer canEngineDbRequestTimer;
freertossupport::StaticOsTimer statusLedTimer;
freertossupport::StaticOsTimer stopGoLedTimer;

hal::CanTxCbk canTxCbk;

LocoNetTx lnTx;

StatusIndicator statusIndicator;
StartStopIndicator startStopIndicator;

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

void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer,
                                    StackType_t** ppxTimerTaskStackBuffer,
                                    uint32_t* pulTimerTaskStackSize) {
  static StackType_t timerTaskStack[configTIMER_TASK_STACK_DEPTH];
  static StaticTask_t timerTaskTcb;
  *ppxTimerTaskTCBBuffer = &timerTaskTcb;
  *ppxTimerTaskStackBuffer = timerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

}  // extern "C"

namespace {
void setup() {
  // Setup I2C & CAN
  halImpl.begin();
  statusIndicator.begin(statusLedTimer, halImpl);
  startStopIndicator.begin(stopGoLedTimer, halImpl);
  hal::beginSerial();
  hal::beginEE();
  hal::beginI2C(dataModel.kMyAddr, routingTask);
  hal::beginCan(routingTask);

  // Setup Serial
  printf("Connect6021Light Initializing...\n");

  // Load Configuration
  printf("Reading Configuration.\n");
  dataModel = hal::LoadConfig();
  routingTask.begin(dataModel, lnTx, stopGoTimer, canEngineDbRequestTimer, statusIndicator);

  // Tie callbacks together
  printf("Setting up callbacks.\n");

  RR32Can::Station::CallbackStruct callbacks;
  callbacks.tx = &canTxCbk;
  callbacks.system = &startStopIndicator;
  callbacks.engine = &routingTask.getCANEngineDB();
  RR32Can::RR32Can.begin(RR32CanUUID, callbacks);

  consoleTask.setup(lnTx);
  printf("Ready!\n");
  ConsoleManager::begin(&dataModel);

  // Start anything timer-related
  routingTask.stopGoStateM_.startRequesting();
  routingTask.canEngineDBStateM_.startRequesting();
}

void setupOsResources() {
  stopGoTimer.Create("StopGo", 1000, true, &routingTask.stopGoStateM_);
  canEngineDbRequestTimer.Create("CanDb", 1000, true, &routingTask.canEngineDBStateM_);
  statusLedTimer.Create("StatusLED", 1000, true, &statusIndicator);
  stopGoLedTimer.Create("StartStop", 1000, true, &startStopIndicator);
}

void setupOsTasks() {
  routingTask.Create("RoutingTask", configMAX_PRIORITIES - 1);
  consoleTask.Create("ConsoleTask",
                     0);  // Lowest prio as this task will always run.
}
}  // namespace

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

extern "C" void notifyLnByteReceived() { routingTask.notifyFromISRWithWake(); }

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

int run_ln_slot_server_dispatch(int argc, const char* const* argv, int argcMatched) {
  static constexpr const char* appName{"LnSlotServerDispatch"};

  if (!checkNumArgs(argc - argcMatched, 1, 1, appName)) {
    display_help(argc, argv);
    return -2;
  }

  const int dispatchAddrInt = atoi(argv[argcMatched]);

  using LocoAddr_t = tasks::RoutingTask::LocoNetSlotServer::LocoAddr_t;
  const LocoAddr_t dispatchAddress = RR32Can::MachineLocomotiveAddress(dispatchAddrInt);
  auto& slotServer = routingTask.getLnSlotServer();
  slotServer.markAddressForDispatch(dispatchAddress);
  return 0;
}

int run_app_download_enginedb_can(int, const char* const*, int) {
  statusIndicator.setCanDbDownload();
  routingTask.getCANEngineDB().fetchEngineDB();
  return 0;
}

int run_app_dump_enginedb_can(int, const char* const*, int) {
  routingTask.getCANEngineDB().dump();
  return 0;
}

}  // namespace ConsoleManager