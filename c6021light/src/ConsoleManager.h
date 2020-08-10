#ifndef __CONSOLEMANAGER_H__
#define __CONSOLEMANAGER_H__

extern "C" {
#include "microrl.h"
}

// Forward delaration for implementation elsewhere.
int run_app_set_turnout_protocol(int argc, const char* const* argv);
int run_app_get_turnout_protocol(int argc, const char* const* argv);

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
