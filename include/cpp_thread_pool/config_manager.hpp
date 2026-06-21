#pragma once

#include "cpp_thread_pool/logger.hpp"

#include <cstddef>
#include <string>

namespace cpp_thread_pool {

struct ThreadPoolConfig {
    std::size_t worker_threads{0};
    LogLevel log_level{LogLevel::info};
    std::string log_file;
};

class ConfigManager {
public:
    static ThreadPoolConfig load(const std::string& path);

private:
    static std::string trim(const std::string& value);
    static LogLevel parse_log_level(const std::string& value);
};

}  // namespace cpp_thread_pool

