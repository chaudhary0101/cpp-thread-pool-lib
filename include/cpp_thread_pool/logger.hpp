#pragma once

#include <fstream>
#include <mutex>
#include <ostream>
#include <string>

namespace cpp_thread_pool {

enum class LogLevel { debug = 0, info = 1, warning = 2, error = 3, off = 4 };

class Logger {
public:
    explicit Logger(LogLevel minimum_level = LogLevel::info);

    void set_level(LogLevel level) noexcept;
    LogLevel level() const noexcept;
    void set_output_file(const std::string& path);
    void log(LogLevel level, const std::string& message);

private:
    static const char* level_name(LogLevel level) noexcept;
    static std::string timestamp();

    mutable std::mutex mutex_;
    LogLevel minimum_level_;
    std::ofstream file_;
    std::ostream* output_;
};

}  // namespace cpp_thread_pool

