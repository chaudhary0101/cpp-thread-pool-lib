#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build-coverage \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCPP_THREAD_POOL_ENABLE_COVERAGE=ON
cmake --build build-coverage --parallel
ctest --test-dir build-coverage --output-on-failure
gcovr --root . --exclude 'tests/' --html-details coverage/index.html
