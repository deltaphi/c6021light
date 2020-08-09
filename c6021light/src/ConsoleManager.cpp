#include "ConsoleManager.h"

#include "hal/PrintfAb.h"

static void microrl_print_cbk(const char* s) {
  printf(s);
  fflush(stdout);
}

static int microrl_execute_callback(int argc, const char* const* argv) {
  printf("microrl_execute: %i args\n", argc);
  return 0;
}

void ConsoleManager::begin() {
  microrl_init(&microrl, microrl_print_cbk);
  microrl_set_execute_callback(&microrl, microrl_execute_callback);
}
