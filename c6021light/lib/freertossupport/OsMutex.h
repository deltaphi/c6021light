#ifndef __FREERTOSSUPPORT__OSMUTEX_H__
#define __FREERTOSSUPPORT__OSMUTEX_H__

#include "FreeRTOS.h"
#include "semphr.h"

namespace freertossupport {

/*
 * \brief Class OsMutex
 */
class OsMutex {
 public:
  OsMutex(const OsMutex&) = default;
  OsMutex& operator=(const OsMutex&) = default;
  OsMutex(OsMutex&&) = delete;
  OsMutex& operator=(OsMutex&&) = delete;

  void lock() { xSemaphoreTake(handle_, portMAX_DELAY); }
  void unlock() { xSemaphoreGive(handle_); }

 protected:
  OsMutex() = default;

  SemaphoreHandle_t handle_;
};

class OsMutexStatic : public OsMutex {
 public:
  OsMutexStatic() = default;

  OsMutexStatic(const OsMutexStatic&) = delete;
  OsMutexStatic& operator=(const OsMutexStatic&) = delete;
  OsMutexStatic(OsMutexStatic&&) = delete;
  OsMutexStatic& operator=(OsMutexStatic&&) = delete;

  void Create() { handle_ = xSemaphoreCreateMutexStatic(&mutexBuffer); }

 private:
  StaticSemaphore_t mutexBuffer;
};
}  // namespace freertossupport

#endif  // __FREERTOSSUPPORT__OSMUTEX_H__
