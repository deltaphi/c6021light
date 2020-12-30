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

void fillCompletionData(char** completionBuffer, std::size_t maxNumCompletions,
                        const cliSupport::Argument* argtable, int argc, const char* const* argv);

}  // namespace cliSupport

#endif  // __CLISUPPORT__CLISUPPORT_H__
