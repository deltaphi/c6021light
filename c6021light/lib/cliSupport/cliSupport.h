#ifndef __CLISUPPORT__CLISUPPORT_H__
#define __CLISUPPORT__CLISUPPORT_H__

#include <cstdint>

namespace cliSupport {

/* Argument Table concept
 *
 * Recursive set of structs. Each struct points to a name and a follow-up struct.
 * Parse through the level of structs to decend in the syntax tree.
 */

using MainFunc_t = int (*)(int argc, const char* const* argv);

struct Argument;

struct Argument {
  const char* name;
  const Argument* options;
};

struct MainFuncName {
  const char* name;
  MainFunc_t mainFunc;
};

/**
 * Finds the longest branch in the tree that matches all arguments.
 */
const Argument* walkArgumentTree(const Argument* argTree, int argc, const char* const* argv);

void fillCompletionData(char** completionData, std::size_t maxNumCompletions, int argc,
                        const char* const* argv, const cliSupport::Argument* argtable);

}  // namespace cliSupport

#endif  // __CLISUPPORT__CLISUPPORT_H__
