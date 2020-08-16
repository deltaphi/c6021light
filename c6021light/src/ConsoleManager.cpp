#include "ConsoleManager.h"

extern "C" {
#include <stdio.h>
#include <string.h>
}

static constexpr const char* app_set_turnout_protocol{"set-protocol"};
static constexpr const char* app_get_turnout_protocol{"get-protocol"};
static constexpr const char* app_help{"help"};
static constexpr const char* app_save{"save"};

#define ISAPP(inp, name) (strncasecmp(inp, name, strlen(name)) == 0)

int run_app_help(int argc, const char* const* argv);
static void microrl_print_cbk(const char* s);
static int microrl_execute_callback(int argc, const char* const* argv);

static void microrl_print_cbk(const char* s) {
  printf(s);
  fflush(stdout);
}

static int microrl_execute_callback(int argc, const char* const* argv) {
  if (argc < 1) {
    return -1;  // No application given.
  }
  if (ISAPP(argv[0], app_help)) {
    return run_app_help(argc, argv);
  } else if (ISAPP(argv[0], app_set_turnout_protocol)) {
    return run_app_set_turnout_protocol(argc, argv);
  } else if (ISAPP(argv[0], app_get_turnout_protocol)) {
    return run_app_get_turnout_protocol(argc, argv);
  } else if (ISAPP(argv[0], app_save)) {
    return run_app_save(argc, argv);
  }

  printf("microrl_execute: %i args\n", argc);
  return 0;
}

int run_app_help(int, const char* const*) {
  printf("Choose one of the following:\n");
  printf("  %s [MM2|DCC] - Set Turnout Protocol.\n", app_set_turnout_protocol);
  printf("  %s - Get current Turnout Protocol.\n", app_get_turnout_protocol);
  printf("  %s - Display this help message.\n", app_help);
  return 0;
}

void ConsoleManager::begin() {
  microrl_init(&microrl, microrl_print_cbk);
  microrl_set_execute_callback(&microrl, microrl_execute_callback);
}
