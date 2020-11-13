#include "ConsoleManager.h"

extern "C" {
#include <stdio.h>
#include <string.h>
}

#define NUM_CONSOLE_APPS (5)

static constexpr const char* app_set_turnout_protocol{"set-protocol"};
static constexpr const char* app_get_turnout_protocol{"get-protocol"};
static constexpr const char* app_help{"help"};
static constexpr const char* app_save{"save"};

const char* all_apps[NUM_CONSOLE_APPS];
char* completion_data[NUM_CONSOLE_APPS];

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

  return -1;
}

static char** microrl_complete_callback(int argc, const char* const* argv) {
  memset(completion_data, 0, sizeof(completion_data));

  if (argc == 0) {
    // Everything is possible
    return const_cast<char**>(all_apps);

  } else if (argc == 1) {
    // Apps are possible
    unsigned int j = 0;
    for (unsigned int i = 0; i < NUM_CONSOLE_APPS; ++i) {
      if (strstr(all_apps[i], argv[0]) != nullptr) {
        completion_data[j] = const_cast<char*>(all_apps[i]);
        ++j;
      }
    }

  } else {
    // Find the app first
    if (ISAPP(argv[0], app_set_turnout_protocol)) {
      if (argc == 2) {
        // App has additional parameters
        char* possible_completions[NUM_CONSOLE_APPS];
        possible_completions[0] = const_cast<char*>(MM2Name);
        possible_completions[1] = const_cast<char*>(DCCName);
        possible_completions[2] = const_cast<char*>(SX1Name);
        possible_completions[3] = nullptr;
        possible_completions[3] = nullptr;

        // remove all that do not match the current command line
        unsigned int j = 0;
        for (unsigned int i = 0; i < NUM_CONSOLE_APPS; ++i) {
          if (strstr(possible_completions[i], argv[1]) != nullptr) {
            completion_data[j] = possible_completions[i];
            ++j;
          }
        }
      }
    }
  }
  return completion_data;
}

int run_app_help(int, const char* const*) {
  printf("Choose one of the following:\n");
  printf("  %s [%s|%s|%s] - Set Turnout Protocol.\n", app_set_turnout_protocol, MM2Name, DCCName,
         SX1Name);
  printf("  %s - Get current Turnout Protocol.\n", app_get_turnout_protocol);
  printf("  %s - Save configuration across reset.\n", app_save);
  printf("  %s - Display this help message.\n", app_help);
  return 0;
}

void ConsoleManager::begin() {
  microrl_init(&microrl, microrl_print_cbk);
  microrl_set_execute_callback(&microrl, microrl_execute_callback);

  all_apps[0] = const_cast<char*>(app_set_turnout_protocol);
  all_apps[1] = const_cast<char*>(app_get_turnout_protocol);
  all_apps[2] = const_cast<char*>(app_help);
  all_apps[3] = const_cast<char*>(app_save);
  all_apps[4] = nullptr;

  microrl_set_complete_callback(&microrl, microrl_complete_callback);
}
