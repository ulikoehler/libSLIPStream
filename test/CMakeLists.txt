cmake_minimum_required(VERSION 3.10)
project(libslip_test CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project root (one level up from test/)
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

include_directories(${PROJECT_ROOT}/include)

# Use system-installed GoogleTest (expect it to be available on the host)
find_package(GTest REQUIRED)
add_executable(test_all
    test_all_main.cpp
    test_slipbuffer.cpp
    test_slipencoder.cpp
    ${PROJECT_ROOT}/src/SLIPBuffer.cpp
    ${PROJECT_ROOT}/src/SLIPStreamDecoder.cpp
    ${PROJECT_ROOT}/src/SLIPStreamEncoder.cpp
)
target_include_directories(test_all PRIVATE ${PROJECT_ROOT}/include)
target_link_libraries(test_all PRIVATE GTest::gtest pthread)
add_test(NAME AllTests COMMAND test_all)
