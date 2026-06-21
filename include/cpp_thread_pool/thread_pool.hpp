#pragma once

#include "cpp_thread_pool/config_manager.hpp"
#include "cpp_thread_pool/logger.hpp"
#include "cpp_thread_pool/metrics_collector.hpp"
#include "cpp_thread_pool/task_queue.hpp"
#include "cpp_thread_pool/task_scheduler.hpp"
#include "cpp_thread_pool/types.hpp"
#include "cpp_thread_pool/worker.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace cpp_thread_pool {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count = 0,
                        LogLevel log_level = LogLevel::info);
    explicit ThreadPool(const ThreadPoolConfig& config);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename Function, typename... Args>
    auto submit(TaskPriority priority, Function&& function, Args&&... args)
        -> std::future<std::invoke_result_t<Function, Args...>>;

    template <typename Function, typename... Args>
    auto submit(Function&& function, Args&&... args)
        -> std::future<std::invoke_result_t<Function, Args...>>;

    template <typename Rep, typename Period, typename Function, typename... Args>
    auto schedule_after(std::chrono::duration<Rep, Period> delay,
                        TaskPriority priority,
                        Function&& function,
                        Args&&... args)
        -> std::pair<TaskId, std::future<std::invoke_result_t<Function, Args...>>>;

    template <typename Rep, typename Period, typename Function>
    TaskId schedule_every(std::chrono::duration<Rep, Period> interval,
                          TaskPriority priority,
                          Function&& function);

    bool cancel_scheduled(TaskId id);
    void resize(std::size_t worker_count);
    void shutdown();
    RuntimeStatistics statistics() const noexcept;
    std::size_t worker_count() const noexcept;
    std::size_t pending_tasks() const;

private:
    void initialize(std::size_t worker_count);
    bool enqueue(TaskId id, TaskPriority priority, std::function<void()> task);
    void worker_loop(std::size_t worker_id);
    TaskId next_task_id() noexcept;

    TaskQueue queue_;
    MetricsCollector metrics_;
    Logger logger_;
    std::vector<std::unique_ptr<Worker>> workers_;
    std::unique_ptr<TaskScheduler> scheduler_;
    std::atomic<TaskId> next_id_{1};
    std::atomic<std::uint64_t> next_sequence_{0};
    std::atomic<std::size_t> desired_workers_{0};
    std::atomic<bool> accepting_{true};
    std::atomic<bool> stopped_{false};
    mutable std::mutex lifecycle_mutex_;
    mutable std::mutex workers_mutex_;
};

template <typename Function, typename... Args>
auto ThreadPool::submit(TaskPriority priority, Function&& function, Args&&... args)
    -> std::future<std::invoke_result_t<Function, Args...>> {
    using Result = std::invoke_result_t<Function, Args...>;
    auto bound = std::bind(std::forward<Function>(function),
                           std::forward<Args>(args)...);
    auto callable = std::make_shared<decltype(bound)>(std::move(bound));
    auto promise = std::make_shared<std::promise<Result>>();
    auto future = promise->get_future();
    const TaskId id = next_task_id();
    if (!enqueue(id, priority, [callable, promise] {
            try {
                if constexpr (std::is_void_v<Result>) {
                    (*callable)();
                    promise->set_value();
                } else {
                    promise->set_value((*callable)());
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        })) {
        throw std::runtime_error("thread pool is not accepting tasks");
    }
    return future;
}

template <typename Function, typename... Args>
auto ThreadPool::submit(Function&& function, Args&&... args)
    -> std::future<std::invoke_result_t<Function, Args...>> {
    return submit(TaskPriority::normal,
                  std::forward<Function>(function),
                  std::forward<Args>(args)...);
}

template <typename Rep, typename Period, typename Function, typename... Args>
auto ThreadPool::schedule_after(std::chrono::duration<Rep, Period> delay,
                                TaskPriority priority,
                                Function&& function,
                                Args&&... args)
    -> std::pair<TaskId, std::future<std::invoke_result_t<Function, Args...>>> {
    using Result = std::invoke_result_t<Function, Args...>;
    auto bound = std::bind(std::forward<Function>(function),
                           std::forward<Args>(args)...);
    auto callable = std::make_shared<decltype(bound)>(std::move(bound));
    auto promise = std::make_shared<std::promise<Result>>();
    auto future = promise->get_future();
    const TaskId id = next_task_id();
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(delay);
    const bool accepted = scheduler_->schedule(
        id, TaskScheduler::Clock::now() + duration, std::chrono::milliseconds{0},
        priority, [callable, promise] {
            try {
                if constexpr (std::is_void_v<Result>) {
                    (*callable)();
                    promise->set_value();
                } else {
                    promise->set_value((*callable)());
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });
    if (!accepted) {
        throw std::runtime_error("task scheduler is stopped");
    }
    return {id, std::move(future)};
}

template <typename Rep, typename Period, typename Function>
TaskId ThreadPool::schedule_every(std::chrono::duration<Rep, Period> interval,
                                  TaskPriority priority,
                                  Function&& function) {
    const auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(interval);
    if (duration.count() <= 0) {
        throw std::invalid_argument("periodic interval must be positive");
    }
    const TaskId id = next_task_id();
    const bool accepted = scheduler_->schedule(
        id, TaskScheduler::Clock::now() + duration, duration, priority,
        std::function<void()>(std::forward<Function>(function)));
    if (!accepted) {
        throw std::runtime_error("task scheduler is stopped");
    }
    return id;
}

}  // namespace cpp_thread_pool
