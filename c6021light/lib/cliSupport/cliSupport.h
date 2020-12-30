#ifndef __CLISUPPORT__CLISUPPORT_H__
#define __CLISUPPORT__CLISUPPORT_H__

#include <cstdint>

namespace cliSupport {

/* Argument Table concept
 *
 * Recursive set of structs. Each struct points to a name and a follow-up struct.
 * Parse through the level of structs to decend in the syntax tree.
 */

/**
 * Function Pointer Type for callback functions handling CLI (sub)commands.
 *
 * \param argc the length of argv.
 * \param argv The argument vector.
 * \param argcMatched How many elements of argv were completely matched before calling this
 * function.
 *
 * \return The return code of the function.
 */
using CliHandlerFunc_t = int (*)(int argc, const char* const* argv, int argcMatched);

constexpr static const int kNoHandler = -128;

struct Argument {
  const char* name = nullptr;
  const Argument* options = nullptr;
  CliHandlerFunc_t handler = nullptr;
  const char* help = nullptr;
};

struct PrefixResult {
  const Argument* arg = nullptr;
  int level = 0;

  bool empty() const { return arg == nullptr && level == 0; }
  bool valid(int argc) const { return arg != nullptr && level < argc; }
};

void fillCompletionData(const char** completionBuffer, std::size_t maxNumCompletions,
                        const cliSupport::Argument* argtable, int argc, const char* const* argv);

int callHandler(const cliSupport::Argument* argtable, int argc, const char* const* argv);

PrefixResult findLongestPrefix(const Argument* argTree, int argc, const char* const* argv);

}  // namespace cliSupport

#endif  // __CLISUPPORT__CLISUPPORT_H__
