cmake_minimum_required(VERSION 3.10)
project(libslip_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project root (one level up from test/)
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

include_directories(${PROJECT_ROOT}/include)

add_executable(test_slipbuffer
    test_slipbuffer.cpp
    ${PROJECT_ROOT}/src/SLIPBuffer.cpp
    ${PROJECT_ROOT}/src/SLIPStream.cpp
)

# Ensure the target can see the public headers
target_include_directories(test_slipbuffer PRIVATE ${PROJECT_ROOT}/include)

# Use system-installed GoogleTest (expect it to be available on the host)
find_package(GTest REQUIRED)
target_link_libraries(test_slipbuffer PRIVATE GTest::gtest_main)

enable_testing()
add_test(NAME SLIPBufferTests COMMAND test_slipbuffer)
