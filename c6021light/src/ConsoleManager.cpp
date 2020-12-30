#include "ConsoleManager.h"

#include <cstdio>
#include <cstring>

#include "RR32Can/Constants.h"
#include "cliSupport.h"

#include "hal/stm32eepromEmulation.h"

#define NUM_COMPLETIONS (6)

#define COMMAND(cmdName) command_##cmdName

#define DEFINE_COMMAND_STRING(cmdName) \
  static constexpr const char* COMMAND(cmdName) { #cmdName }

#define COMMAND_ARGS(cmdName) args_##cmdName

DEFINE_COMMAND_STRING(config);

DEFINE_COMMAND_STRING(get);
DEFINE_COMMAND_STRING(set);

DEFINE_COMMAND_STRING(turnoutProtocol);

DEFINE_COMMAND_STRING(lnSlotServer);

DEFINE_COMMAND_STRING(flash);
DEFINE_COMMAND_STRING(dump);
DEFINE_COMMAND_STRING(save);
DEFINE_COMMAND_STRING(help);

namespace ConsoleManager {

DataModel* dataModel_;
microrl_t microrl;

int run_app_set_turnout_protocol(int argc, const char* const* argv, int argcMatched);
int run_app_get_turnout_protocol(int argc, const char* const* argv, int argcMatched);
int run_app_save(int argc, const char* const* argv, int argcMatched);
int run_app_help(int argc, const char* const* argv, int argcMatched);
int run_app_dump_flash(int argc, const char* const* argv,
                       int argcMatched);  // implemented in eeprom emulation

// Arguments for config
static const cliSupport::Argument turnoutProtocolArguments[] = {
    {MM2Name, nullptr, nullptr}, {DCCName, nullptr, nullptr}, {SX1Name, nullptr, nullptr}, {}};

static const cliSupport::Argument COMMAND_ARGS(config_get)[] = {
    {COMMAND(turnoutProtocol), turnoutProtocolArguments, run_app_get_turnout_protocol}, {}};

static const cliSupport::Argument COMMAND_ARGS(config_set)[] = {
    {COMMAND(turnoutProtocol), turnoutProtocolArguments, run_app_set_turnout_protocol}, {}};

static const cliSupport::Argument COMMAND_ARGS(config)[] = {
    {COMMAND(get), COMMAND_ARGS(config_get), nullptr},
    {COMMAND(set), COMMAND_ARGS(config_set), nullptr},
    {}};

// Arguments for flash
static const cliSupport::Argument COMMAND_ARGS(flash)[] = {
    {COMMAND(dump), nullptr, run_app_dump_flash}, {COMMAND(save), nullptr, run_app_save}, {}};

// Top-Level Arguments
static const cliSupport::Argument argtable[] = {{COMMAND(config), COMMAND_ARGS(config), nullptr},
                                                {COMMAND(help), nullptr, run_app_help},
                                                {COMMAND(flash), COMMAND_ARGS(flash), nullptr},
                                                {}};

static void microrl_print_cbk(const char* s);
static int microrl_execute_callback(int argc, const char* const* argv);

static void microrl_print_cbk(const char* s) {
  printf(s);
  fflush(stdout);
}

static int microrl_execute_callback(int argc, const char* const* argv) {
  int result = cliSupport::callHandler(argtable, argc, argv);
  if (result == cliSupport::kNoHandler) {
    printf("No handler for command \"");
    for (int i = 0; i < argc; ++i) {
      printf("%s ", argv[i]);
    }
    printf("\"\n");
    run_app_help(argc, argv, 0);
  }
  return result;
}

static char** microrl_complete_callback(int argc, const char* const* argv) {
  /// Static buffer for the completion data passed to microrl.
  static char* completion_data[NUM_COMPLETIONS];
  cliSupport::fillCompletionData(completion_data, NUM_COMPLETIONS - 1, argtable, argc, argv);
  return completion_data;
}

int run_app_help(int, const char* const*, int) {
  printf("Choose one of the following:\n");
  printf("  %s [%s|%s|%s] - Set Turnout Protocol.\n", COMMAND(config), MM2Name, DCCName, SX1Name);
  printf("  %s - Get current Turnout Protocol.\n", COMMAND(config));
  printf("  %s %s - Save configuration across reset.\n", COMMAND(flash), COMMAND(save));
  printf("  %s %s - Show contents of EEPROM Emulation Flash.\n", COMMAND(flash), COMMAND(dump));
  printf("  %s - Display this help message.\n", COMMAND(help));

  return 0;
}

void begin(DataModel* dataModel) {
  dataModel_ = dataModel;

  microrl_init(&microrl, microrl_print_cbk);
  microrl_set_execute_callback(&microrl, microrl_execute_callback);
  microrl_set_complete_callback(&microrl, microrl_complete_callback);
}

bool checkNumArgs(int numArgs, int lower, int upper, const char* appName) {
  static constexpr const char* formatString = "%s: Too %s arguments (%i expected, %i given).\n";
  if (numArgs < lower) {
    printf(formatString, appName, "few", lower, numArgs);
    return false;
  } else if (numArgs > upper) {
    printf(formatString, appName, "many", lower, numArgs);
    return false;
  } else {
    return true;
  }
}

int run_app_set_turnout_protocol(int argc, const char* const* argv, int argcMatched) {
  static constexpr const char* text{": Set Turnout protocol to "};
  static constexpr const char* appName{"SetTurnoutProtocol"};

  if (!checkNumArgs(argc - argcMatched, 1, 1, appName)) {
    return -2;
  }

  int protocolArgumentIdx = argcMatched;
  const char* protocolArgument = argv[protocolArgumentIdx];

  if (strncasecmp(protocolArgument, MM2Name, strlen(MM2Name)) == 0) {
    dataModel_->accessoryRailProtocol = RR32Can::RailProtocol::MM2;
    printf("%s%s'%s'.\n", appName, text, protocolArgument);
  } else if (strncasecmp(protocolArgument, DCCName, strlen(DCCName)) == 0) {
    dataModel_->accessoryRailProtocol = RR32Can::RailProtocol::DCC;
    printf("%s%s'%s'.\n", appName, text, argv[argcMatched + 1]);
  } else if (strncasecmp(protocolArgument, SX1Name, strlen(SX1Name)) == 0) {
    dataModel_->accessoryRailProtocol = RR32Can::RailProtocol::SX1;
    printf("%s%s'%s'.\n", appName, text, protocolArgument);
  } else {
    printf("%s: Unknown rail protocol '%s'.\n", appName, protocolArgument);
    return -3;
  }

  return 0;
}

int run_app_get_turnout_protocol(int argc, const char* const* argv, int argcMatched) {
  static constexpr const char* appName{"GetTurnoutProtocol"};

  if (!checkNumArgs(argc - argcMatched, 0, 0, appName)) {
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

int run_app_save(int argc, const char* const* argv, int argcMatched) {
  static constexpr const char* appName{"SaveToFlash"};

  if (!checkNumArgs(argc - argcMatched, 0, 0, appName)) {
    return -2;
  }

  hal::SaveConfig(*dataModel_);
  printf("%s: Configuration saved to flash.\n", argv[0]);

  return 0;
}

}  // namespace ConsoleManager