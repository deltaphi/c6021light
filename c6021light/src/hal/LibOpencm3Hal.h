#ifndef __HAL__LIBOPENCM3HAL_H__
#define __HAL__LIBOPENCM3HAL_H__

#include <atomic>

#include "AtomicRingBuffer/AtomicRingBuffer.h"

#include "ConsoleManager.h"
#include "DataModel.h"

#include "OsQueue.h"

namespace hal {

/*
 * \brief Class LibOpencm3Hal
 */
class LibOpencm3Hal {
 public:
  void begin(ConsoleManager* console) {
    console_ = console;
    beginClock();
    beginGpio();
    beginSerial();
    beginLocoNet();
    beginEE();
  }

  void loopSerial();

  void led(bool on);
  void toggleLed();

  void SaveConfig(const DataModel& dataModel);
  DataModel LoadConfig();

  uint8_t SerialWrite(char* ptr, AtomicRingBuffer::AtomicRingBuffer::size_type len);

  static LibOpencm3Hal* instance_;
  void irqSerialTxDMA();

 private:
  void beginClock();
  void beginGpio();
  void beginSerial();
  void beginLocoNet();
  void beginEE();

  void startSerialTx();

  ConsoleManager* console_;

  constexpr static const AtomicRingBuffer::AtomicRingBuffer::size_type bufferSize_ = 1024;
  AtomicRingBuffer::AtomicRingBuffer serialBuffer_;
  uint8_t bufferMemory_[bufferSize_];

  std::atomic_bool serialDmaBusy_;

  AtomicRingBuffer::AtomicRingBuffer::pointer_type serialMem_;
  AtomicRingBuffer::AtomicRingBuffer::size_type serialNumBytes_;
};

}  // namespace hal

#endif  // __HAL__LIBOPENCM3HAL_H__
