#include "cpp_thread_pool/task_scheduler.hpp"

#include <utility>

namespace cpp_thread_pool {

TaskScheduler::TaskScheduler(SubmitCallback callback)
    : submit_(std::move(callback)), scheduler_thread_([this] { run(); }) {}

TaskScheduler::~TaskScheduler() {
    shutdown();
}

bool TaskScheduler::schedule(TaskId id,
                             Clock::time_point due,
                             std::chrono::milliseconds interval,
                             TaskPriority priority,
                             std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return false;
        }
        tasks_.push(
            ScheduledTask{id, due, interval, priority, std::move(task),
                          next_sequence_++});
    }
    condition_.notify_one();
    return true;
}

bool TaskScheduler::cancel(TaskId id) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return false;
        }
        cancelled_.insert(id);
    }
    condition_.notify_one();
    return true;
}

void TaskScheduler::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }
    condition_.notify_all();
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
}

void TaskScheduler::run() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stopping_) {
        if (tasks_.empty()) {
            condition_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            continue;
        }

        const auto due = tasks_.top().due;
        if (condition_.wait_until(lock, due, [this, due] {
                return stopping_ || tasks_.empty() || tasks_.top().due < due;
            })) {
            continue;
        }
        if (stopping_ || tasks_.empty()) {
            continue;
        }

        ScheduledTask task = tasks_.top();
        tasks_.pop();
        if (cancelled_.erase(task.id) != 0) {
            continue;
        }

        lock.unlock();
        const bool accepted = submit_(task.id, task.priority, std::move(task.function));
        lock.lock();

        if (accepted && task.interval.count() > 0 && !stopping_ &&
            !cancelled_.contains(task.id)) {
            task.due = Clock::now() + task.interval;
            task.sequence = next_sequence_++;
            tasks_.push(std::move(task));
        }
    }
    while (!tasks_.empty()) {
        tasks_.pop();
    }
    cancelled_.clear();
}

}  // namespace cpp_thread_pool

