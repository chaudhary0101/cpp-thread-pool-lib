#include "cpp_thread_pool/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace cpp_thread_pool {

Logger::Logger(LogLevel minimum_level)
    : minimum_level_(minimum_level), output_(&std::clog) {}

void Logger::set_level(LogLevel level) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    minimum_level_ = level;
}

LogLevel Logger::level() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return minimum_level_;
}

void Logger::set_output_file(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    file_.open(path, std::ios::out | std::ios::app);
    if (!file_) {
        throw std::runtime_error("unable to open log file: " + path);
    }
    output_ = &file_;
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (minimum_level_ == LogLevel::off ||
        static_cast<int>(level) < static_cast<int>(minimum_level_)) {
        return;
    }
    (*output_) << timestamp() << " [" << level_name(level) << "] [thread "
               << std::this_thread::get_id() << "] " << message << '\n';
    output_->flush();
}

const char* Logger::level_name(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::debug:
            return "DEBUG";
        case LogLevel::info:
            return "INFO";
        case LogLevel::warning:
            return "WARN";
        case LogLevel::error:
            return "ERROR";
        case LogLevel::off:
            return "OFF";
    }
    return "UNKNOWN";
}

std::string Logger::timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) %
        1000;
    std::tm local_time{};
#ifdef _MSC_VER
    localtime_s(&local_time, &time);
#else
    localtime_r(&time, &local_time);
#endif
    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << '.'
           << std::setfill('0') << std::setw(3) << milliseconds.count();
    return stream.str();
}

}  // namespace cpp_thread_pool
