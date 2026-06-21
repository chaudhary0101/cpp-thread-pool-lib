# Suggested Git history

Apply these commits in order while publishing the repository:

1. `chore: initialize CMake project and repository metadata`
2. `feat: add thread-safe prioritized task queue`
3. `feat: introduce RAII worker abstraction`
4. `feat: implement futures-based task submission`
5. `feat: add graceful queue draining and pool shutdown`
6. `feat: support dynamic worker pool resizing`
7. `feat: add delayed and periodic task scheduler`
8. `feat: implement scheduled task cancellation`
9. `feat: add thread-safe structured logger`
10. `feat: collect runtime task and latency metrics`
11. `feat: load pool settings from config files`
12. `test: cover queue ordering and thread pool behavior`
13. `test: add mixed-workload integration tests`
14. `ci: add compiler matrix, static analysis, and Valgrind`
15. `docs: publish architecture, performance, and operations guides`

Each commit should compile independently. Keep formatting-only changes separate
from behavioral changes when extending the project.

