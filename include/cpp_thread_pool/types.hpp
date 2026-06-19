#pragma once

#include <cstdint>

namespace cpp_thread_pool {

enum class TaskPriority : std::uint8_t {
    low = 0,
    normal = 1,
    high = 2,
    critical = 3
};

using TaskId = std::uint64_t;

}  // namespace cpp_thread_pool

