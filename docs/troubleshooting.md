# Troubleshooting

## CMake cannot download Google Test

Tests use `FetchContent`. Confirm outbound HTTPS access or configure without tests:

```bash
cmake -S . -B build -DCPP_THREAD_POOL_BUILD_TESTS=OFF
```

For an offline build, provide a package cache or set `FETCHCONTENT_SOURCE_DIR_GOOGLETEST`
to an existing Google Test source directory.

## A future never becomes ready

Do not wait on one pool task from another task when every worker could be occupied
by the waiting tasks. That dependency pattern can starve the pool. Resolve the
dependency outside the pool or provision enough workers for the dependency graph.

## Periodic task runs once after cancellation

Cancellation is cooperative. A task already transferred from the scheduler to the
worker queue may execute once. Cancellation prevents future rescheduling.

## Shutdown does not wait for a delayed task

This is intentional. Shutdown drains immediate tasks already accepted by the worker
queue but discards timers that have not become due. Otherwise a distant or periodic
timer could prevent termination.

## Lower-priority task runs before a critical task

Priority only orders queued work. A lower-priority task that a worker has already
started is not preempted.

## Valgrind reports library allocations

Build in Debug mode and run the supplied script. If Google Test or the C++ runtime
appears in a report, inspect the full origin before suppressing it:

```bash
./scripts/run_valgrind.sh
```

## ThreadSanitizer

On GCC or Clang:

```bash
cmake -S . -B build-tsan \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
cmake --build build-tsan --parallel
ctest --test-dir build-tsan --output-on-failure
```

## Too many log messages

Set `log_level=warning`, `error`, or `off` in the config. Logging is serialized and
can distort microbenchmarks, so benchmark with logging disabled.

