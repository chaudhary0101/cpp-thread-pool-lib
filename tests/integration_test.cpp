#include "cpp_thread_pool/thread_pool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <numeric>
#include <vector>

namespace {

TEST(IntegrationTest, HandlesConcurrentMixedWorkloadAndReportsMetrics) {
    cpp_thread_pool::ThreadPool pool(4, cpp_thread_pool::LogLevel::off);
    std::vector<std::future<int>> results;
    for (int value = 1; value <= 250; ++value) {
        const auto priority = value % 10 == 0
                                  ? cpp_thread_pool::TaskPriority::high
                                  : cpp_thread_pool::TaskPriority::normal;
        results.push_back(pool.submit(priority, [value] { return value; }));
    }

    int sum = 0;
    for (auto& result : results) {
        sum += result.get();
    }
    EXPECT_EQ(sum, 31375);

    pool.shutdown();
    const auto stats = pool.statistics();
    EXPECT_EQ(stats.submitted_tasks, 250U);
    EXPECT_EQ(stats.completed_tasks, 250U);
    EXPECT_EQ(stats.queued_tasks, 0U);
    EXPECT_GT(stats.uptime_seconds, 0.0);
}

TEST(IntegrationTest, GracefulShutdownDrainsQueuedTasks) {
    cpp_thread_pool::ThreadPool pool(2, cpp_thread_pool::LogLevel::off);
    std::atomic<int> completed{0};
    for (int index = 0; index < 100; ++index) {
        pool.submit([&completed] { completed.fetch_add(1); });
    }
    pool.shutdown();
    EXPECT_EQ(completed.load(), 100);
}

}  // namespace

