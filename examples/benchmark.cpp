#include "cpp_thread_pool/thread_pool.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    const std::size_t task_count =
        argc > 1 ? static_cast<std::size_t>(std::strtoull(argv[1], nullptr, 10))
                 : 100000;
    cpp_thread_pool::ThreadPool pool;
    std::vector<std::future<std::uint64_t>> results;
    results.reserve(task_count);

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < task_count; ++index) {
        results.push_back(pool.submit([index] {
            const auto value = static_cast<std::uint64_t>(index);
            return value * value;
        }));
    }
    std::uint64_t checksum = 0;
    for (auto& result : results) {
        checksum ^= result.get();
    }
    const auto elapsed =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
            .count();

    std::cout << "workers: " << pool.worker_count() << '\n'
              << "tasks: " << task_count << '\n'
              << "elapsed_seconds: " << std::fixed << std::setprecision(6)
              << elapsed << '\n'
              << "throughput_tasks_per_second: "
              << static_cast<double>(task_count) / elapsed << '\n'
              << "checksum: " << checksum << '\n';
}

