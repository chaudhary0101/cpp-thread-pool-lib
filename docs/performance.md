# Performance

## Measurement goals

The benchmark measures submission, scheduling, future synchronization, and worker
execution as one end-to-end operation. It uses small CPU tasks to expose framework
overhead rather than application work.

## Run the benchmark

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCPP_THREAD_POOL_BUILD_TESTS=OFF
cmake --build build --parallel
./build/benchmark 1000000
```

Record the CPU model, logical core count, compiler, optimization level, kernel, and
power mode with every result. Compare builds on an otherwise idle host.

```bash
lscpu
uname -a
g++ --version
taskset -c 0-7 ./build/benchmark 1000000
```

## Result template

The executable prints machine-readable labels for worker count, task count,
elapsed seconds, tasks per second, and a deterministic checksum. A typical record
has this shape:

```text
workers: 8
tasks: 1000000
elapsed_seconds: 1.842731
throughput_tasks_per_second: 542672.841390
checksum: 174969942272
```

The numeric block illustrates the output format and is not a cross-machine
performance claim. Run the command on the target Linux machine and place its actual
output, together with host metadata, in a release or pull request so results remain
reproducible and hardware-specific.

## Profiling

Use Linux `perf` to identify lock contention, allocation cost, and scheduler
overhead:

```bash
perf stat -r 5 ./build/benchmark 1000000
perf record -g ./build/benchmark 1000000
perf report
```

Useful counters include task-clock, context switches, CPU migrations, cache misses,
and instructions per cycle.

## Expected scaling behavior

Compute-heavy independent tasks should scale until physical cores are saturated.
Very small tasks eventually become dominated by queue locking, allocation, and
future synchronization. Blocking tasks can benefit from more workers than physical
cores, but unbounded growth should be avoided.

## Optimization opportunities

Measured workloads may justify per-worker queues, work stealing, task object
pooling, cache-line padding for hot atomics, or move-only callable support from a
newer language standard. These are intentionally excluded from the current API to
keep behavior predictable and C++17-compatible.
