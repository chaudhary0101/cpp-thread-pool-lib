#include "cpp_thread_pool/task_queue.hpp"

#include <utility>

namespace cpp_thread_pool {

bool TaskQueue::push(Task task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return false;
        }
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
    return true;
}

bool TaskQueue::wait_pop(Task& task, const std::function<bool()>& should_stop) {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this, &should_stop] {
        return closed_ || !tasks_.empty() || should_stop();
    });
    if (should_stop() || (closed_ && tasks_.empty())) {
        return false;
    }
    task = std::move(const_cast<Task&>(tasks_.top()));
    tasks_.pop();
    return true;
}

void TaskQueue::wake_all() {
    condition_.notify_all();
}

void TaskQueue::close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    condition_.notify_all();
}

bool TaskQueue::closed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

std::size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

}  // namespace cpp_thread_pool
