#ifndef __CONSOLEMANAGER_H__
#define __CONSOLEMANAGER_H__

extern "C" {
#include "microrl.h"
}

#include "DataModel.h"

constexpr const char* MM2Name = "MM2";
constexpr const char* DCCName = "DCC";
constexpr const char* SX1Name = "SX1";

/*
 * \brief Class ConsoleManager
 */
namespace ConsoleManager {

extern microrl_t microrl;
void begin(DataModel* dataModel);

void display_help(int argc, const char* const* argv);
void display_version(int argc, const char* const* argv);

/**
 * \brief Verify that the expected (range of) number(s) of arguments is available.
 *
 * \param argc The number of remaining arguments.
 * \param lower The minimum number of expected arguments.
 * \param upper The maximum number of expected arguments.
 * \param appName The name of the application to be used in the error message.
 */
bool checkNumArgs(int argc, int lower, int upper, const char* appName);
}  // namespace ConsoleManager

#endif  // __CONSOLEMANAGER_H__
