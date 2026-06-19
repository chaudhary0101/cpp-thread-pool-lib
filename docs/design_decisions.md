# Design decisions

## C++17 and STL only

The runtime library depends only on C++17, the STL, and POSIX-compatible threading
through `std::thread`. Google Test is isolated to the test target. This keeps the
library straightforward to integrate into Linux services, embedded user-space
applications, and automotive or telecom components.

## Priority queue with stable ordering

Tasks carry both a priority and a monotonically increasing sequence number.
Critical work preempts queued lower-priority work, while equal-priority tasks remain
FIFO. Sequence values avoid relying on pointer address or timing resolution.

## Futures through packaged tasks

`std::packaged_task` provides type-safe results for arbitrary return values,
including `void`, and automatically stores user exceptions in the returned future.
The worker remains alive when user code throws.

## One scheduler thread

A dedicated timing thread avoids making every worker aware of deadlines. The
scheduler performs no user work; it only transfers due tasks to the pool. This
keeps timer latency independent from task duration and avoids blocking workers on
future deadlines.

## Cooperative resizing

Growth creates workers immediately. Shrink requests lower an atomic desired count
and wake sleeping workers. Excess workers complete their current task and exit,
which avoids unsafe thread cancellation.

## Graceful rather than abrupt shutdown

Accepted immediate tasks are drained. Scheduled tasks that have not yet reached
their deadline are discarded when the scheduler stops. This policy prevents a
shutdown from waiting indefinitely for a distant or periodic timer.

## Atomic metrics

Counters use `memory_order_relaxed`; they are observations, not synchronization
primitives. Queue size and worker ownership are obtained from their respective
protected structures. Metrics are intentionally lightweight enough for production
diagnostics.

## Explicit ownership

The pool owns workers and the scheduler through `std::unique_ptr`. Callables own
their packaged state through `std::shared_ptr` only because `std::function` requires
copyable targets in C++17. No raw owning pointers are used.

## Configuration format

A small `key=value` parser was chosen over JSON/YAML to honor the no-runtime-
dependency constraint. Supported keys are documented in the sample config and
unknown keys are ignored for forward compatibility.

## Known trade-offs

- A single global queue is simple and provides strict priority semantics, but a
  work-stealing design may scale better at very high core counts.
- Periodic execution uses fixed-delay scheduling: the next run is calculated after
  dispatch. This avoids bursty catch-up behavior.
- Priority is not preemptive. A running low-priority task is never interrupted.

