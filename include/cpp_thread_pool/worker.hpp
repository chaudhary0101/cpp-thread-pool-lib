#pragma once

#include <cstddef>
#include <functional>
#include <thread>

namespace cpp_thread_pool {

class Worker {
public:
    using WorkLoop = std::function<void(std::size_t)>;

    Worker(std::size_t id, WorkLoop loop);
    ~Worker();

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    void join();
    std::size_t id() const noexcept;

private:
    std::size_t id_;
    std::thread thread_;
};

}  // namespace cpp_thread_pool

