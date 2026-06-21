#include "cpp_thread_pool/metrics_collector.hpp"

namespace cpp_thread_pool {

MetricsCollector::MetricsCollector() : started_at_(std::chrono::steady_clock::now()) {}

void MetricsCollector::task_submitted() noexcept {
    submitted_.fetch_add(1, std::memory_order_relaxed);
}

void MetricsCollector::task_completed(std::chrono::nanoseconds duration) noexcept {
    completed_.fetch_add(1, std::memory_order_relaxed);
    execution_ns_.fetch_add(static_cast<std::uint64_t>(duration.count()),
                            std::memory_order_relaxed);
}

void MetricsCollector::task_failed(std::chrono::nanoseconds duration) noexcept {
    failed_.fetch_add(1, std::memory_order_relaxed);
    execution_ns_.fetch_add(static_cast<std::uint64_t>(duration.count()),
                            std::memory_order_relaxed);
}

void MetricsCollector::task_rejected() noexcept {
    rejected_.fetch_add(1, std::memory_order_relaxed);
}

RuntimeStatistics MetricsCollector::snapshot(std::size_t workers,
                                             std::size_t queued) const noexcept {
    RuntimeStatistics statistics;
    statistics.submitted_tasks = submitted_.load(std::memory_order_relaxed);
    statistics.completed_tasks = completed_.load(std::memory_order_relaxed);
    statistics.failed_tasks = failed_.load(std::memory_order_relaxed);
    statistics.rejected_tasks = rejected_.load(std::memory_order_relaxed);
    statistics.total_execution_time_ns =
        execution_ns_.load(std::memory_order_relaxed);
    statistics.active_workers = workers;
    statistics.queued_tasks = queued;
    const auto executed = statistics.completed_tasks + statistics.failed_tasks;
    if (executed != 0) {
        statistics.average_execution_time_us =
            static_cast<double>(statistics.total_execution_time_ns) /
            static_cast<double>(executed) / 1000.0;
    }
    statistics.uptime_seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - started_at_)
            .count();
    return statistics;
}

}  // namespace cpp_thread_pool

