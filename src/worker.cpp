#include "cpp_thread_pool/worker.hpp"

#include <utility>

namespace cpp_thread_pool {

Worker::Worker(std::size_t id, WorkLoop loop)
    : id_(id), thread_([id, loop = std::move(loop)] { loop(id); }) {}

Worker::~Worker() {
    join();
}

void Worker::join() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

std::size_t Worker::id() const noexcept {
    return id_;
}

}  // namespace cpp_thread_pool

