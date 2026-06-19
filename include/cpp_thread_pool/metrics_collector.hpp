#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace cpp_thread_pool {

struct RuntimeStatistics {
    std::uint64_t submitted_tasks{};
    std::uint64_t completed_tasks{};
    std::uint64_t failed_tasks{};
    std::uint64_t rejected_tasks{};
    std::uint64_t total_execution_time_ns{};
    std::size_t active_workers{};
    std::size_t queued_tasks{};
    double average_execution_time_us{};
    double uptime_seconds{};
};

class MetricsCollector {
public:
    MetricsCollector();

    void task_submitted() noexcept;
    void task_completed(std::chrono::nanoseconds duration) noexcept;
    void task_failed(std::chrono::nanoseconds duration) noexcept;
    void task_rejected() noexcept;
    RuntimeStatistics snapshot(std::size_t workers, std::size_t queued) const noexcept;

private:
    std::atomic<std::uint64_t> submitted_{0};
    std::atomic<std::uint64_t> completed_{0};
    std::atomic<std::uint64_t> failed_{0};
    std::atomic<std::uint64_t> rejected_{0};
    std::atomic<std::uint64_t> execution_ns_{0};
    std::chrono::steady_clock::time_point started_at_;
};

}  // namespace cpp_thread_pool

