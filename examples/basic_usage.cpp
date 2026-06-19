#include "cpp_thread_pool/thread_pool.hpp"

#include <iostream>
#include <string>
#include <vector>

int main() {
    using cpp_thread_pool::TaskPriority;
    using cpp_thread_pool::ThreadPool;

    ThreadPool pool(4);
    auto sum = pool.submit(TaskPriority::high, [](int lhs, int rhs) {
        return lhs + rhs;
    }, 20, 22);
    auto message = pool.submit([] {
        return std::string{"work completed on a worker thread"};
    });

    std::vector<std::future<int>> squares;
    for (int value = 1; value <= 8; ++value) {
        squares.push_back(pool.submit([value] { return value * value; }));
    }

    std::cout << "sum: " << sum.get() << '\n';
    std::cout << message.get() << '\n';
    std::cout << "squares:";
    for (auto& result : squares) {
        std::cout << ' ' << result.get();
    }
    std::cout << '\n';

    const auto stats = pool.statistics();
    std::cout << "submitted=" << stats.submitted_tasks
              << " completed=" << stats.completed_tasks
              << " average_us=" << stats.average_execution_time_us << '\n';
}

