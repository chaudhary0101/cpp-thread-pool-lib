#include "cpp_thread_pool/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    using namespace std::chrono_literals;
    using cpp_thread_pool::TaskPriority;
    using cpp_thread_pool::ThreadPool;

    ThreadPool pool(2);
    auto delayed = pool.schedule_after(150ms, TaskPriority::high, [] {
        return 42;
    });

    std::atomic<int> heartbeat_count{0};
    const auto heartbeat_id =
        pool.schedule_every(50ms, TaskPriority::normal, [&heartbeat_count] {
            heartbeat_count.fetch_add(1);
        });

    std::cout << "delayed result: " << delayed.second.get() << '\n';
    std::this_thread::sleep_for(180ms);
    pool.cancel_scheduled(heartbeat_id);
    std::cout << "periodic executions: " << heartbeat_count.load() << '\n';
}

