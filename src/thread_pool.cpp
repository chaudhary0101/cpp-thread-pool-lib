#include "cpp_thread_pool/thread_pool.hpp"

#include <chrono>
#include <exception>
#include <thread>
#include <utility>

namespace cpp_thread_pool {
namespace {

std::size_t normalized_worker_count(std::size_t requested) {
    if (requested != 0) {
        return requested;
    }
    const auto detected = std::thread::hardware_concurrency();
    return detected == 0 ? 1U : static_cast<std::size_t>(detected);
}

}  // namespace

ThreadPool::ThreadPool(std::size_t worker_count, LogLevel log_level)
    : logger_(log_level) {
    initialize(normalized_worker_count(worker_count));
}

ThreadPool::ThreadPool(const ThreadPoolConfig& config)
    : logger_(config.log_level) {
    if (!config.log_file.empty()) {
        logger_.set_output_file(config.log_file);
    }
    initialize(normalized_worker_count(config.worker_threads));
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::initialize(std::size_t worker_count) {
    desired_workers_.store(worker_count, std::memory_order_release);
    scheduler_ = std::make_unique<TaskScheduler>(
        [this](TaskId id, TaskPriority priority, std::function<void()> task) {
            return enqueue(id, priority, std::move(task));
        });
    workers_.reserve(worker_count);
    for (std::size_t id = 0; id < worker_count; ++id) {
        workers_.push_back(std::make_unique<Worker>(
            id, [this](std::size_t worker_id) { worker_loop(worker_id); }));
    }
    logger_.log(LogLevel::info,
                "thread pool started with " + std::to_string(worker_count) +
                    " workers");
}

bool ThreadPool::enqueue(TaskId id,
                         TaskPriority priority,
                         std::function<void()> task) {
    if (!accepting_.load(std::memory_order_acquire)) {
        metrics_.task_rejected();
        return false;
    }
    Task queued{id, priority,
                next_sequence_.fetch_add(1, std::memory_order_relaxed),
                std::move(task)};
    if (!queue_.push(std::move(queued))) {
        metrics_.task_rejected();
        return false;
    }
    metrics_.task_submitted();
    return true;
}

void ThreadPool::worker_loop(std::size_t worker_id) {
    logger_.log(LogLevel::debug,
                "worker " + std::to_string(worker_id) + " started");
    Task task;
    const auto should_stop = [this, worker_id] {
        return worker_id >= desired_workers_.load(std::memory_order_acquire);
    };
    while (queue_.wait_pop(task, should_stop)) {
        const auto started = std::chrono::steady_clock::now();
        try {
            task.function();
            metrics_.task_completed(std::chrono::steady_clock::now() - started);
        } catch (const std::exception& exception) {
            metrics_.task_failed(std::chrono::steady_clock::now() - started);
            logger_.log(LogLevel::error,
                        "task " + std::to_string(task.id) +
                            " failed: " + exception.what());
        } catch (...) {
            metrics_.task_failed(std::chrono::steady_clock::now() - started);
            logger_.log(LogLevel::error,
                        "task " + std::to_string(task.id) +
                            " failed with an unknown exception");
        }
    }
    logger_.log(LogLevel::debug,
                "worker " + std::to_string(worker_id) + " stopped");
}

TaskId ThreadPool::next_task_id() noexcept {
    return next_id_.fetch_add(1, std::memory_order_relaxed);
}

bool ThreadPool::cancel_scheduled(TaskId id) {
    return scheduler_ != nullptr && scheduler_->cancel(id);
}

void ThreadPool::resize(std::size_t worker_count) {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    if (worker_count == 0) {
        throw std::invalid_argument("worker_count must be greater than zero");
    }
    if (!accepting_.load(std::memory_order_acquire)) {
        throw std::runtime_error("cannot resize a stopped thread pool");
    }
    std::vector<std::unique_ptr<Worker>> retired_workers;
    std::size_t current = 0;
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        if (!accepting_.load(std::memory_order_acquire)) {
            throw std::runtime_error("cannot resize a stopped thread pool");
        }
        current = workers_.size();
        if (worker_count == current) {
            return;
        }
        if (worker_count > current) {
            desired_workers_.store(worker_count, std::memory_order_release);
            workers_.reserve(worker_count);
            for (std::size_t id = current; id < worker_count; ++id) {
                workers_.push_back(std::make_unique<Worker>(
                    id,
                    [this](std::size_t worker_id) { worker_loop(worker_id); }));
            }
        } else {
            desired_workers_.store(worker_count, std::memory_order_release);
            for (std::size_t id = worker_count; id < current; ++id) {
                retired_workers.push_back(std::move(workers_[id]));
            }
            workers_.resize(worker_count);
        }
    }
    if (!retired_workers.empty()) {
        queue_.wake_all();
        for (auto& worker : retired_workers) {
            worker->join();
        }
    }
    logger_.log(LogLevel::info,
                "thread pool resized from " + std::to_string(current) + " to " +
                    std::to_string(worker_count) + " workers");
}

void ThreadPool::shutdown() {
    std::lock_guard<std::mutex> lifecycle_lock(lifecycle_mutex_);
    bool expected = false;
    if (!stopped_.compare_exchange_strong(expected, true,
                                          std::memory_order_acq_rel)) {
        return;
    }
    accepting_.store(false, std::memory_order_release);
    if (scheduler_) {
        scheduler_->shutdown();
    }
    std::vector<std::unique_ptr<Worker>> workers;
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        desired_workers_.store(workers_.size(), std::memory_order_release);
        workers.swap(workers_);
    }
    queue_.close();
    for (auto& worker : workers) {
        worker->join();
    }
    logger_.log(LogLevel::info, "thread pool shut down gracefully");
}

RuntimeStatistics ThreadPool::statistics() const noexcept {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    return metrics_.snapshot(workers_.size(), queue_.size());
}

std::size_t ThreadPool::worker_count() const noexcept {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    return workers_.size();
}

std::size_t ThreadPool::pending_tasks() const {
    return queue_.size();
}

}  // namespace cpp_thread_pool
