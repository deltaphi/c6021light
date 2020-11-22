#ifndef __FREERTOSSUPPORT__OSQUEUE_H__
#define __FREERTOSSUPPORT__OSQUEUE_H__

#include <type_traits>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "queue.h"

namespace freertossupport {

template <typename E>
class OsQueue {
 public:
  using QueueElement = E;

  struct SendResult {
    BaseType_t errorCode;
    BaseType_t higherPriorityTaskWoken;
  };

  struct ReceiveResult {
    BaseType_t errorCode;
    QueueElement element;
  };

  OsQueue() : handle_(NULL){};

  OsQueue(QueueHandle_t handle) : handle_(handle) { configASSERT(handle_ != NULL); }

  static_assert(std::is_trivially_copyable<E>::value, "E not copyable.");

  // QueueHandle_t getHandle() const { return handle_; }

  SendResult SendFromISR(const QueueElement& element) {
    configASSERT(handle_ != NULL);
    SendResult result;
    result.errorCode = xQueueSendFromISR(handle_, &element, &result.higherPriorityTaskWoken);
    return result;
  }

  SendResult Send(const QueueElement& element) {
    configASSERT(handle_ != NULL);
    SendResult result;
    result.errorCode = xQueueSend(handle_, &element, result.higherPriorityTaskWoken);
    return result;
  }

  ReceiveResult Receive(TickType_t ticksToWait) {
    configASSERT(handle_ != NULL);
    ReceiveResult result;
    result.errorCode = xQueueReceive(handle_, &result.element, ticksToWait);
    return result;
  }

 protected:
  QueueHandle_t handle_;
};

/*
 * \brief Class StaticOsQueue
 */
template <typename E, uint32_t L>
class StaticOsQueue : public OsQueue<E> {
 public:
  using QueueElement = typename OsQueue<E>::QueueElement;
  constexpr static const auto kQueueLength = L;

  StaticOsQueue()
      : OsQueue<E>(xQueueCreateStatic(kQueueLength, sizeof(QueueElement), buffer, &queue)) {}

 private:
  uint8_t buffer[kQueueLength * sizeof(QueueElement)];
  StaticQueue_t queue;
};

}  // namespace freertossupport

#endif  // __FREERTOSSUPPORT__OSQUEUE_H__
