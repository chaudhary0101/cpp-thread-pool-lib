#include "cpp_thread_pool/task_queue.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

TEST(TaskQueueTest, PopsHigherPriorityBeforeLowerPriority) {
    cpp_thread_pool::TaskQueue queue;
    std::vector<int> order;
    queue.push({1, cpp_thread_pool::TaskPriority::low, 0,
                [&order] { order.push_back(1); }});
    queue.push({2, cpp_thread_pool::TaskPriority::critical, 1,
                [&order] { order.push_back(2); }});

    cpp_thread_pool::Task task;
    ASSERT_TRUE(queue.wait_pop(task, [] { return false; }));
    task.function();
    ASSERT_TRUE(queue.wait_pop(task, [] { return false; }));
    task.function();
    EXPECT_EQ(order, (std::vector<int>{2, 1}));
}

TEST(TaskQueueTest, PreservesFifoWithinPriority) {
    cpp_thread_pool::TaskQueue queue;
    queue.push({1, cpp_thread_pool::TaskPriority::normal, 10, [] {}});
    queue.push({2, cpp_thread_pool::TaskPriority::normal, 11, [] {}});

    cpp_thread_pool::Task first;
    cpp_thread_pool::Task second;
    ASSERT_TRUE(queue.wait_pop(first, [] { return false; }));
    ASSERT_TRUE(queue.wait_pop(second, [] { return false; }));
    EXPECT_EQ(first.id, 1U);
    EXPECT_EQ(second.id, 2U);
}

TEST(TaskQueueTest, RejectsPushAfterClose) {
    cpp_thread_pool::TaskQueue queue;
    queue.close();
    EXPECT_FALSE(queue.push({1, cpp_thread_pool::TaskPriority::normal, 0, [] {}}));
}

}  // namespace

