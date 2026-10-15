#!/bin/sh

set -eu

cd "$(dirname "$0")/.."

git ls-files -z -- \
  '*.c' '*.h' '*.cc' '*.hh' '*.cpp' '*.hpp' '*.cxx' '*.hxx' \
  ':!c6021light/lib/freertos/**' \
  ':!c6021light/lib/microrl/**' \
  | xargs -0 uvx --from clang-format==18.1.8 clang-format --style=file -i
