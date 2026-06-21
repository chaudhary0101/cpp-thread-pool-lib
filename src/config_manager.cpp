#include "cpp_thread_pool/config_manager.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace cpp_thread_pool {

ThreadPoolConfig ConfigManager::load(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("unable to open config file: " + path);
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        line = trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';') {
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("invalid config entry at line " +
                                     std::to_string(line_number));
        }
        values[trim(line.substr(0, separator))] =
            trim(line.substr(separator + 1));
    }

    ThreadPoolConfig config;
    const auto worker = values.find("worker_threads");
    if (worker != values.end()) {
        const auto parsed = std::stoull(worker->second);
        if (parsed == 0) {
            throw std::runtime_error("worker_threads must be greater than zero");
        }
        config.worker_threads = static_cast<std::size_t>(parsed);
    }
    const auto level = values.find("log_level");
    if (level != values.end()) {
        config.log_level = parse_log_level(level->second);
    }
    const auto file = values.find("log_file");
    if (file != values.end()) {
        config.log_file = file->second;
    }
    return config;
}

std::string ConfigManager::trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](char c) {
        return std::isspace(static_cast<unsigned char>(c)) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](char c) {
                          return std::isspace(static_cast<unsigned char>(c)) != 0;
                      }).base();
    return first < last ? std::string(first, last) : std::string{};
}

LogLevel ConfigManager::parse_log_level(const std::string& value) {
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](char c) {
                       return static_cast<char>(
                           std::tolower(static_cast<unsigned char>(c)));
                   });
    if (normalized == "debug") return LogLevel::debug;
    if (normalized == "info") return LogLevel::info;
    if (normalized == "warning" || normalized == "warn") return LogLevel::warning;
    if (normalized == "error") return LogLevel::error;
    if (normalized == "off") return LogLevel::off;
    throw std::runtime_error("invalid log_level: " + value);
}

}  // namespace cpp_thread_pool

