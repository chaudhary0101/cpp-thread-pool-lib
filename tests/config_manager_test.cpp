#include "cpp_thread_pool/config_manager.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace {

TEST(ConfigManagerTest, LoadsSupportedValues) {
    const std::string path = "thread_pool_test.conf";
    {
        std::ofstream output(path);
        output << "worker_threads=3\n"
               << "log_level=warning\n"
               << "log_file=pool-test.log\n";
    }
    const auto config = cpp_thread_pool::ConfigManager::load(path);
    std::remove(path.c_str());

    EXPECT_EQ(config.worker_threads, 3U);
    EXPECT_EQ(config.log_level, cpp_thread_pool::LogLevel::warning);
    EXPECT_EQ(config.log_file, "pool-test.log");
}

TEST(ConfigManagerTest, RejectsMalformedInput) {
    const std::string path = "invalid_thread_pool_test.conf";
    {
        std::ofstream output(path);
        output << "this is not valid\n";
    }
    EXPECT_THROW(cpp_thread_pool::ConfigManager::load(path), std::runtime_error);
    std::remove(path.c_str());
}

}  // namespace

