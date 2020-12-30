#include "cliSupport.h"

#include <cstring>

namespace cliSupport {

/**
 * Type for a callback that tells findLongestPrefix what properties we are looking for in
 * the PrefixResult.
 */
using PrefixPredicate_t = bool (*)(const Argument*);

struct PrefixResult {
  const Argument* arg = nullptr;
  int level = 0;

  bool empty() const { return arg == nullptr && level == 0; }
  bool valid(int argc) const { return arg != nullptr && level < argc; }
};

/**
 * Finds the longest branch in the tree that perfectly matches all arguments.
 *
 * There may be parts of argv with partial or no matches left.
 *
 * \param argTree The root Argument* list to start parsing argv with.
 * \param argc Length of argv
 * \param argv The command line parameter tokens
 * \param predicate A predicate function that tells the tree walk which encountered nodes are
 * interesting.
 *
 * \return Pointer to an Argument and level on which this argument was encountered. Always returns
 * the deepest Argument that matched predicate.
 */
PrefixResult findLongestPrefix(const Argument* argTree, int argc, const char* const* argv,
                               PrefixPredicate_t predicate);

/**
 * Predicate function for findLongestPrefix that is interested in every node that matched the
 * argTree.
 */
bool TruePredicate(const Argument*) { return true; }

bool HasHandlerPredicate(const Argument* arg) { return arg->handler != nullptr; }

void setResultIfPredicateHolds(PrefixResult& result, PrefixPredicate_t predicate,
                               decltype(PrefixResult::arg) argument,
                               decltype(PrefixResult::level) level) {
  if (predicate(argument)) {
    result.arg = argument;
    result.level = level;
  }
}

PrefixResult findLongestPrefix(const Argument* argtable, int argc, const char* const* argv,
                               PrefixPredicate_t predicate) {
  PrefixResult result;
  const Argument* currentLevelArguments = argtable;

  for (int argvIdx = 0; argvIdx < argc && currentLevelArguments != nullptr; ++argvIdx) {
    // Find all candidates for the next level
    bool furtherDescendPossible = false;

    for (const Argument* argument = currentLevelArguments; argument->name != nullptr; ++argument) {
      if (strcmp(argument->name, argv[argvIdx]) == 0) {
        setResultIfPredicateHolds(result, predicate, argument, argvIdx);
        currentLevelArguments = argument->options;  // Decend a level
        furtherDescendPossible = true;
        break;  // Skip remainder of this level, advance to the next level.
      }
    }

    if (!furtherDescendPossible) {
      // No candidate for further descend found - abort search
      return result;
    }
  }
  return result;
}

void fillCompletionData(const char** completionBuffer, std::size_t maxNumCompletions,
                        const cliSupport::Argument* argtable, int argc, const char* const* argv) {
  // completions: Parse through the tree which args match. When an arg matches completely and there
  // are more args, consider the following elements.

  memset(completionBuffer, 0, maxNumCompletions * sizeof(char*));

  if (argc < 1) {
    // Nothing to match.
    return;
  }

  PrefixResult prefix = findLongestPrefix(argtable, argc, argv, TruePredicate);

  if (prefix.level <= argc) {
    const char* argvToMatch = nullptr;
    const Argument* argumentIt = nullptr;

    if (prefix.empty()) {
      argumentIt = argtable;
      argvToMatch = argv[0];
    } else if (prefix.valid(argc)) {
      argumentIt = prefix.arg->options;
      argvToMatch = argv[prefix.level + 1];
    }

    std::size_t completionIt = 0;
    while ((argumentIt->name != nullptr) && (completionIt < maxNumCompletions)) {
      if (strstr(argumentIt->name, argvToMatch) == argumentIt->name) {
        completionBuffer[completionIt] = argumentIt->name;
        ++completionIt;
      }
      ++argumentIt;
    }
  }
}

int callHandler(const cliSupport::Argument* argtable, int argc, const char* const* argv) {
  PrefixResult prefix = findLongestPrefix(argtable, argc, argv, HasHandlerPredicate);

  if (prefix.valid(argc)) {
    return prefix.arg->handler(argc, argv, prefix.level + 1);
  } else {
    return kNoHandler;
  }
}

}  // namespace cliSupport
