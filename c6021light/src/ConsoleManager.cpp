#include "ConsoleManager.h"

#include <cstdio>
#include <cstring>

#include "RR32Can/Constants.h"

#include "hal/stm32eepromEmulation.h"

/* Argument Table concept
 *
 * Recursive set of structs. Each struct points to a name and a follow-up struct.
 * Parse through the level of structs to decend in the syntax tree.
 */

#define NUM_COMPLETIONS (6)

struct Argument;

struct Argument {
  const char* name;
  const Argument* options;
};

static constexpr const char* app_set_turnout_protocol{"set-protocol"};
static constexpr const char* app_get_turnout_protocol{"get-protocol"};
static constexpr const char* app_help{"help"};
static constexpr const char* app_save{"save"};
static constexpr const char* app_dump_flash{"dump-flash"};

// Arguments for set-protocol
static const Argument configArguments[] = {
    {MM2Name, nullptr}, {DCCName, nullptr}, {SX1Name, nullptr}, {nullptr, nullptr}};

// Top-Level Arguments
static const Argument argtable[] = {{app_set_turnout_protocol, configArguments},
                                    {app_get_turnout_protocol, nullptr},
                                    {app_help, nullptr},
                                    {app_save, nullptr},
                                    {app_dump_flash, nullptr},
                                    {nullptr, nullptr}};

/// Static buffer for the completion data passed to microrl.
char* completion_data[NUM_COMPLETIONS];

#define ISAPP(inp, name) (strncasecmp(inp, name, strlen(name)) == 0)

namespace ConsoleManager {

DataModel* dataModel_;
microrl_t microrl;

const Argument* walkArgumentTree(const Argument* argTree, int argc, const char* const* argv);
void fillCompletionData(int argc, const char* const* argv);

int run_app_set_turnout_protocol(int argc, const char* const* argv);
int run_app_get_turnout_protocol(int argc, const char* const* argv);
int run_app_save(int argc, const char* const* argv);
int run_app_help(int argc, const char* const* argv);
int run_app_dump_flash(int argc, const char* const* argv);  // implemented in eeprom emulation
static void microrl_print_cbk(const char* s);
static int microrl_execute_callback(int argc, const char* const* argv);

static void microrl_print_cbk(const char* s) {
  printf(s);
  fflush(stdout);
}

/**
 * Finds the longest branch in the tree that matches all arguments.
 */
const Argument* walkArgumentTree(const Argument* argTree, int& firstUnparsedArgc, int argc,
                                 const char* const* argv) {
  const Argument* nextLevelCandidate = argTree;
  for (firstUnparsedArgc = 0; firstUnparsedArgc < argc && nextLevelCandidate != nullptr;
       ++firstUnparsedArgc) {
    // Find all candidates for the next level
    bool parseSuccess = false;
    for (const Argument* levelIt = nextLevelCandidate; levelIt->name != nullptr; ++levelIt) {
      if (strcmp(levelIt->name, argv[firstUnparsedArgc]) == 0) {
        nextLevelCandidate = levelIt->options;
        parseSuccess = true;
        break;  // Advance to the next level
      }
    }
    if (!parseSuccess) {
      // No candidate found - abort search
      return nextLevelCandidate;
    }
  }
  return nextLevelCandidate;
}

void fillCompletionData(int argc, const char* const* argv) {
  // completions: Parse through the tree which args match. When an arg matches completely and there
  // are more args, consider the following elements.

  int firstUnparsedArgc = 0;
  const Argument* candidateLevel = walkArgumentTree(argtable, firstUnparsedArgc, argc, argv);

  memset(completion_data, 0, sizeof(completion_data));

  if (firstUnparsedArgc < argc && candidateLevel != nullptr) {
    int completionDataIt = 0;
    for (const Argument* levelIt = candidateLevel; levelIt->name != nullptr; ++levelIt) {
      if (strstr(levelIt->name, argv[firstUnparsedArgc]) == levelIt->name) {
        completion_data[completionDataIt] = const_cast<char*>(levelIt->name);
        ++completionDataIt;
      }
    }
  }
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
  } else if (ISAPP(argv[0], app_dump_flash)) {
    return run_app_dump_flash(argc, argv);
  }

  return -1;
}

static char** microrl_complete_callback(int argc, const char* const* argv) {
  fillCompletionData(argc, argv);
  return completion_data;
}

int run_app_help(int, const char* const*) {
  printf("Choose one of the following:\n");
  printf("  %s [%s|%s|%s] - Set Turnout Protocol.\n", app_set_turnout_protocol, MM2Name, DCCName,
         SX1Name);
  printf("  %s - Get current Turnout Protocol.\n", app_get_turnout_protocol);
  printf("  %s - Save configuration across reset.\n", app_save);
  printf("  %s - Display this help message.\n", app_help);
  printf("  %s - Show contents of EEPROM Emulation Flash.\n", app_dump_flash);
  return 0;
}

void begin(DataModel* dataModel) {
  dataModel_ = dataModel;

  microrl_init(&microrl, microrl_print_cbk);
  microrl_set_execute_callback(&microrl, microrl_execute_callback);
  microrl_set_complete_callback(&microrl, microrl_complete_callback);
}

int run_app_set_turnout_protocol(int argc, const char* const* argv) {
  static constexpr const char* text{": Set Turnout protocol to "};
  if (argc < 2) {
    printf("%s: Too few arguments (1 expected).\n", argv[0]);
    return -2;
  } else if (argc > 2) {
    printf("%s: Too many arguments (1 expected).\n", argv[0]);
    return -2;
  }

  if (strncasecmp(argv[1], MM2Name, strlen(MM2Name)) == 0) {
    dataModel_->accessoryRailProtocol = RR32Can::RailProtocol::MM2;
    printf("%s%s'%s'.\n", argv[0], text, argv[1]);
  } else if (strncasecmp(argv[1], DCCName, strlen(DCCName)) == 0) {
    dataModel_->accessoryRailProtocol = RR32Can::RailProtocol::DCC;
    printf("%s%s'%s'.\n", argv[0], text, argv[1]);
  } else if (strncasecmp(argv[1], SX1Name, strlen(SX1Name)) == 0) {
    dataModel_->accessoryRailProtocol = RR32Can::RailProtocol::SX1;
    printf("%s%s'%s'.\n", argv[0], text, argv[1]);
  } else {
    printf("%s: Unknown rail protocol '%s'.\n", argv[0], argv[1]);
    return -3;
  }

  return 0;
}

int run_app_get_turnout_protocol(int argc, const char* const* argv) {
  if (argc > 1) {
    printf("%s: Too many arguments (0 expected).\n", argv[0]);
    return -2;
  }

  const char* turnoutProtocol = nullptr;

  switch (dataModel_->accessoryRailProtocol) {
    case RR32Can::RailProtocol::MM1:
    case RR32Can::RailProtocol::MM2:
    case RR32Can::RailProtocol::MFX:
    case RR32Can::RailProtocol::UNKNOWN:
      turnoutProtocol = MM2Name;
      break;
    case RR32Can::RailProtocol::DCC:
      turnoutProtocol = DCCName;
      break;
    case RR32Can::RailProtocol::SX1:
    case RR32Can::RailProtocol::SX2:
      turnoutProtocol = SX1Name;
      break;
  }

  printf("%s: The current turnout protocol is %s.\n", argv[0], turnoutProtocol);

  return 0;
}

int run_app_save(int argc, const char* const* argv) {
  if (argc > 1) {
    printf("%s: Too many arguments (0 expected).\n", argv[0]);
    return -2;
  }

  hal::SaveConfig(*dataModel_);
  printf("%s: Configuration saved to flash.\n", argv[0]);

  return 0;
}

}  // namespace ConsoleManager