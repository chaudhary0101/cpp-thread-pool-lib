#include "cpp_thread_pool/thread_pool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

TEST(ThreadPoolTest, ReturnsValuesThroughFutures) {
    cpp_thread_pool::ThreadPool pool(2, cpp_thread_pool::LogLevel::off);
    auto result = pool.submit([](int value) { return value * 2; }, 21);
    EXPECT_EQ(result.get(), 42);
}

TEST(ThreadPoolTest, PropagatesTaskExceptionsThroughFuture) {
    cpp_thread_pool::ThreadPool pool(1, cpp_thread_pool::LogLevel::off);
    auto result = pool.submit([]() -> int {
        throw std::runtime_error("expected failure");
    });
    EXPECT_THROW(result.get(), std::runtime_error);
}

TEST(ThreadPoolTest, ExecutesAllSubmittedTasks) {
    cpp_thread_pool::ThreadPool pool(4, cpp_thread_pool::LogLevel::off);
    std::atomic<int> count{0};
    std::vector<std::future<void>> futures;
    for (int index = 0; index < 1000; ++index) {
        futures.push_back(pool.submit([&count] { count.fetch_add(1); }));
    }
    for (auto& future : futures) {
        future.get();
    }
    EXPECT_EQ(count.load(), 1000);
}

TEST(ThreadPoolTest, SupportsDynamicResize) {
    cpp_thread_pool::ThreadPool pool(2, cpp_thread_pool::LogLevel::off);
    pool.resize(4);
    EXPECT_EQ(pool.worker_count(), 4U);
    pool.resize(1);
    EXPECT_EQ(pool.worker_count(), 1U);
    EXPECT_EQ(pool.submit([] { return 7; }).get(), 7);
}

TEST(ThreadPoolTest, RunsDelayedTasks) {
    cpp_thread_pool::ThreadPool pool(1, cpp_thread_pool::LogLevel::off);
    const auto start = std::chrono::steady_clock::now();
    auto scheduled = pool.schedule_after(
        40ms, cpp_thread_pool::TaskPriority::normal, [] { return 9; });
    EXPECT_EQ(scheduled.second.get(), 9);
    EXPECT_GE(std::chrono::steady_clock::now() - start, 35ms);
}

TEST(ThreadPoolTest, CancelsPeriodicTasks) {
    cpp_thread_pool::ThreadPool pool(2, cpp_thread_pool::LogLevel::off);
    std::atomic<int> runs{0};
    const auto id = pool.schedule_every(
        10ms, cpp_thread_pool::TaskPriority::normal,
        [&runs] { runs.fetch_add(1); });
    std::this_thread::sleep_for(45ms);
    EXPECT_TRUE(pool.cancel_scheduled(id));
    const int count_after_cancel = runs.load();
    std::this_thread::sleep_for(35ms);
    EXPECT_LE(runs.load(), count_after_cancel + 1);
}

}  // namespace

