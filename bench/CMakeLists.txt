cmake_minimum_required(VERSION 3.10)

project(benchmarks CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Google Benchmark
find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found, benchmarks will not be built")
    return()
endif()

# Find Google Test (needed for some benchmark utilities)
find_package(GTest QUIET)

# Include parent directory for source files
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Benchmark executable
add_executable(bench_all
    bench_main.cpp
    bench_buffer.cpp
    bench_encoder.cpp
    bench_decoder.cpp
    bench_crc32.cpp
    ${PROJECT_ROOT}/src/Buffer.cpp
    ${PROJECT_ROOT}/src/Encoder.cpp
    ${PROJECT_ROOT}/src/Decoder.cpp
    ${PROJECT_ROOT}/src/Error.cpp
    ${PROJECT_ROOT}/src/CRC32.cpp
)

target_include_directories(bench_all PRIVATE ${PROJECT_ROOT}/include)
target_link_libraries(bench_all PRIVATE benchmark::benchmark benchmark::benchmark_main)

if(GTest_FOUND)
    target_link_libraries(bench_all PRIVATE GTest::gtest)
endif()

# Enable optimization for better benchmark accuracy
target_compile_options(bench_all PRIVATE -O3 -march=native)

# Add custom target to run benchmarks
add_custom_target(run_bench
    COMMAND bench_all
    DEPENDS bench_all
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running benchmarks"
)
