#pragma once

#include "cpp_thread_pool/types.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

namespace cpp_thread_pool {

struct Task {
    TaskId id{};
    TaskPriority priority{TaskPriority::normal};
    std::uint64_t sequence{};
    std::function<void()> function;
};

class TaskQueue {
public:
    bool push(Task task);
    bool wait_pop(Task& task, const std::function<bool()>& should_stop);
    void wake_all();
    void close();
    bool closed() const;
    std::size_t size() const;

private:
    struct Compare {
        bool operator()(const Task& lhs, const Task& rhs) const noexcept {
            if (lhs.priority != rhs.priority) {
                return static_cast<std::uint8_t>(lhs.priority) <
                       static_cast<std::uint8_t>(rhs.priority);
            }
            return lhs.sequence > rhs.sequence;
        }
    };

    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::priority_queue<Task, std::vector<Task>, Compare> tasks_;
    bool closed_{false};
};

}  // namespace cpp_thread_pool
