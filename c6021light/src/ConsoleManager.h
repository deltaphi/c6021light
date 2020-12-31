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
bool checkNumArgs(int argc, int lower, int upper, const char* appName);
}  // namespace ConsoleManager

#endif  // __CONSOLEMANAGER_H__
