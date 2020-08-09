#ifndef __CONSOLEMANAGER_H__
#define __CONSOLEMANAGER_H__

extern "C" {
#include "microrl.h"
}

/*
 * \brief Class ConsoleManager
 */
class ConsoleManager {
 public:
  void begin();
  microrl_t* getMicroRl() { return &microrl; };

 private:
  microrl_t microrl;
};

#endif  // __CONSOLEMANAGER_H__
