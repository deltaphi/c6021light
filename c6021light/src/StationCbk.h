#ifndef __STATIONCBK_H__
#define __STATIONCBK_H__

#include <hal/LibOpencm3Hal.h>
#include "RR32Can/callback/SystemCbk.h"

/*
 * \brief Class AccessoryCbk
 */
class AccessoryCbk : public RR32Can::callback::SystemCbk {
 public:
  void begin(hal::LibOpencm3Hal& hal);

  /**
   * \brief Set whether the system is on (true) or off (false).
   *
   * \param onOff Whether the system is on (true) or off (false).
   * \param response Whether the packet was a response (true) or a request (false).
   */
  void setSystemState(bool onOff, bool response) override;

 private:
  hal::LibOpencm3Hal* hal_ = nullptr;
};

#endif  // __STATIONCBK_H__
