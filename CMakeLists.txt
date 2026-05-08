# This CMakeLists.txt is intentionally an ESP-IDF component file.
# Use `test/CMakeLists.txt` for standalone C++ test builds.
# Use `bench/CMakeLists.txt` for standalone C++ benchmark builds.

cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS "src/Decoder.cpp" "src/Encoder.cpp" "src/Buffer.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES driver)

# Add benchmark subdirectory if building standalone
if(NOT IDF_PROJECT)
    add_subdirectory(bench)
endif()
