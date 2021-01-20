#ifndef __HAL__DEBOUNCE_H__
#define __HAL__DEBOUNCE_H__

#include <cstdint>

#include "FreeRTOS.h"
#include "task.h"

namespace hal {

/*
 * \brief Class Debounce
 */
class Debounce {
 public:
  enum class Edge { FALLING = 0, RISING = 1 };
  enum class Level { LOW = 0, HIGH = 1 };

  /**
   * Add a detected edge to the debounce algorithm.
   *
   * \return Whether the edge influenced the debounced value.
   */
  bool addEdge(Edge edge) {
    if (!isCooldownActive()) {
      applyEdge(edge);
      startCooldown();
      return true;
    } else {
      return false;
    }
  }

  Level getDebouncedStatus() const { return debouncedLevel_; }

 private:
  using Cooldown_t = uint32_t;

  constexpr static const Cooldown_t kCooldownDuration_ms = 100;

  Level debouncedLevel_;
  Cooldown_t cooldownTimeout_;

  Cooldown_t now() const {
    const auto ticks = xTaskGetTickCount();
    const Cooldown_t millis = ticks * portTICK_PERIOD_MS;
    return millis;
  }

  bool isCooldownActive() const { return now() < cooldownTimeout_; }

  void startCooldown() { cooldownTimeout_ = now() + kCooldownDuration_ms; }

  void applyEdge(Edge edge) { debouncedLevel_ = static_cast<Level>(edge); }
};

}  // namespace hal

#endif  // __HAL__DEBOUNCE_H__
