#pragma once

#include "cpp_thread_pool/types.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

namespace cpp_thread_pool {

class TaskScheduler {
public:
    using Clock = std::chrono::steady_clock;
    using SubmitCallback =
        std::function<bool(TaskId, TaskPriority, std::function<void()>)>;

    explicit TaskScheduler(SubmitCallback callback);
    ~TaskScheduler();

    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;

    bool schedule(TaskId id,
                  Clock::time_point due,
                  std::chrono::milliseconds interval,
                  TaskPriority priority,
                  std::function<void()> task);
    bool cancel(TaskId id);
    void shutdown();

private:
    struct ScheduledTask {
        TaskId id{};
        Clock::time_point due;
        std::chrono::milliseconds interval{0};
        TaskPriority priority{TaskPriority::normal};
        std::function<void()> function;
        std::uint64_t sequence{};
    };

    struct Compare {
        bool operator()(const ScheduledTask& lhs,
                        const ScheduledTask& rhs) const noexcept {
            if (lhs.due != rhs.due) {
                return lhs.due > rhs.due;
            }
            return lhs.sequence > rhs.sequence;
        }
    };

    void run();

    SubmitCallback submit_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, Compare> tasks_;
    std::unordered_set<TaskId> cancelled_;
    std::thread scheduler_thread_;
    std::uint64_t next_sequence_{0};
    bool stopping_{false};
};

}  // namespace cpp_thread_pool

