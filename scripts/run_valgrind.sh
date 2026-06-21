#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build-valgrind -DCMAKE_BUILD_TYPE=Debug
cmake --build build-valgrind --parallel
valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --error-exitcode=1 \
  ./build-valgrind/cpp_thread_pool_tests

