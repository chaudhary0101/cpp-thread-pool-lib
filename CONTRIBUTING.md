# Contributing

Create a focused branch from `main`, add tests for behavioral changes, and keep the
public API C++17-compatible.

Before opening a pull request:

```bash
clang-format -i include/cpp_thread_pool/*.hpp src/*.cpp tests/*.cpp examples/*.cpp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Describe the concurrency invariant affected by the change, include benchmark
results for performance claims, and run Valgrind or ThreadSanitizer for changes to
ownership or synchronization.

