#include "cliSupport.h"

#include <cstring>

namespace cliSupport {

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

void fillCompletionData(char** completionData, std::size_t maxNumCompletions, int argc,
                        const char* const* argv, const cliSupport::Argument* argtable) {
  // completions: Parse through the tree which args match. When an arg matches completely and there
  // are more args, consider the following elements.

  int firstUnparsedArgc = 0;
  const Argument* candidateLevel = walkArgumentTree(argtable, firstUnparsedArgc, argc, argv);

  memset(completionData, 0, maxNumCompletions);

  if (firstUnparsedArgc < argc && candidateLevel != nullptr) {
    std::size_t completionDataIt = 0;
    for (const Argument* levelIt = candidateLevel;
         (levelIt->name != nullptr) && (completionDataIt < maxNumCompletions); ++levelIt) {
      if (strstr(levelIt->name, argv[firstUnparsedArgc]) == levelIt->name) {
        completionData[completionDataIt] = const_cast<char*>(levelIt->name);
        ++completionDataIt;
      }
    }
  }
}

}  // namespace cliSupport
