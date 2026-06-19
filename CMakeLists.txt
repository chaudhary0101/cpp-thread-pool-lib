cmake_minimum_required(VERSION 3.16)
project(cpp_thread_pool_lib VERSION 1.0.0 LANGUAGES CXX)

option(CPP_THREAD_POOL_BUILD_TESTS "Build tests" ON)
option(CPP_THREAD_POOL_BUILD_EXAMPLES "Build examples" ON)
option(CPP_THREAD_POOL_ENABLE_COVERAGE "Enable GCC/Clang coverage flags" OFF)
option(CPP_THREAD_POOL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_library(cpp_thread_pool
    src/config_manager.cpp
    src/logger.cpp
    src/metrics_collector.cpp
    src/task_queue.cpp
    src/task_scheduler.cpp
    src/thread_pool.cpp
    src/worker.cpp)
add_library(cpp_thread_pool::cpp_thread_pool ALIAS cpp_thread_pool)

target_include_directories(cpp_thread_pool
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_link_libraries(cpp_thread_pool PUBLIC Threads::Threads)
target_compile_features(cpp_thread_pool PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(cpp_thread_pool PRIVATE /W4 /permissive-)
    if(CPP_THREAD_POOL_WARNINGS_AS_ERRORS)
        target_compile_options(cpp_thread_pool PRIVATE /WX)
    endif()
else()
    target_compile_options(cpp_thread_pool PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(CPP_THREAD_POOL_WARNINGS_AS_ERRORS)
        target_compile_options(cpp_thread_pool PRIVATE -Werror)
    endif()
endif()

if(CPP_THREAD_POOL_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cpp_thread_pool PUBLIC --coverage -O0 -g)
    target_link_options(cpp_thread_pool PUBLIC --coverage)
endif()

if(CPP_THREAD_POOL_BUILD_EXAMPLES)
    add_executable(basic_usage examples/basic_usage.cpp)
    target_link_libraries(basic_usage PRIVATE cpp_thread_pool::cpp_thread_pool)

    add_executable(scheduled_tasks examples/scheduled_tasks.cpp)
    target_link_libraries(scheduled_tasks PRIVATE cpp_thread_pool::cpp_thread_pool)

    add_executable(benchmark examples/benchmark.cpp)
    target_link_libraries(benchmark PRIVATE cpp_thread_pool::cpp_thread_pool)
endif()

include(CTest)
if(BUILD_TESTING AND CPP_THREAD_POOL_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(cpp_thread_pool_tests
        tests/config_manager_test.cpp
        tests/integration_test.cpp
        tests/task_queue_test.cpp
        tests/thread_pool_test.cpp)
    target_link_libraries(cpp_thread_pool_tests
        PRIVATE cpp_thread_pool::cpp_thread_pool GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(cpp_thread_pool_tests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(TARGETS cpp_thread_pool
    EXPORT cpp_thread_pool_targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT cpp_thread_pool_targets
    FILE cpp_thread_pool_targets.cmake
    NAMESPACE cpp_thread_pool::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp_thread_pool)
configure_package_config_file(
    cmake/cpp_thread_pool_config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cpp_thread_pool_config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp_thread_pool)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cpp_thread_pool_config_version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cpp_thread_pool_config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cpp_thread_pool_config_version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cpp_thread_pool)
