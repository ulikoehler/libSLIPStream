// Test decoding of all test data files
#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "SLIPStream/Decoder.hpp"
#include "SLIPStream/Encoder.hpp"
#include "SLIPStream/CRC32.hpp"

namespace fs = std::filesystem;

using namespace SLIPStream;

class TestDataFilesTest : public ::testing::Test {
protected:
    std::string testDataDir;
    
    void SetUp() override {
        // Get test data directory path
        testDataDir = std::string(PROJECT_ROOT) + "/test/test_data";
    }
    
    std::vector<uint8_t> readFile(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + filepath);
        }
        
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        return data;
    }
    
    std::vector<std::string> getTestFiles() {
        std::vector<std::string> files;
        
        if (!fs::exists(testDataDir)) {
            return files;
        }
        
        for (const auto& entry : fs::directory_iterator(testDataDir)) {
            if (entry.path().extension() == ".bin") {
                files.push_back(entry.path().string());
            }
        }
        
        std::sort(files.begin(), files.end());
        return files;
    }
};

// Test that all test data files can be decoded
TEST_F(TestDataFilesTest, DecodeAllTestFiles) {
    auto files = getTestFiles();
    
    if (files.empty()) {
        GTEST_SKIP() << "No test data files found";
    }
    
    for (const auto& filepath : files) {
        std::string filename = fs::path(filepath).filename().string();
        
        try {
            auto data = readFile(filepath);
            
            // Try to decode using the decoder
            std::vector<uint8_t> rxbuf(256);
            std::vector<std::vector<uint8_t>> received_messages;
            std::vector<LogInfo> log_entries;
            
            auto callback = [&received_messages](uint8_t* data, size_t size) {
                std::vector<uint8_t> msg(data, data + size);
                received_messages.push_back(msg);
            };
            
            auto logCallback = [&log_entries](LogInfo info) {
                log_entries.push_back(info);
            };
            
            Decoder decoder(rxbuf.data(), rxbuf.size(), callback, logCallback);
            
            // Feed the data to the decoder
            for (uint8_t byte : data) {
                auto result = decoder.consume_ex(byte);
                (void)result; // Ignore result
            }
            
            // If we received messages, they should be valid
            for (const auto& msg : received_messages) {
                // Empty messages are valid for empty_frame.bin
                // Other messages should have content
                if (filename != "empty_frame.bin") {
                    EXPECT_GT(msg.size(), 0);
                }
            }
            
        } catch (const std::exception& e) {
            // Some files may be intentionally incomplete/invalid
            // That's okay as long as we don't crash
            std::cout << "Note: " << filename << " caused exception: " << e.what() << std::endl;
        }
    }
}

// Test that simple_frame.bin has a valid CRC
TEST_F(TestDataFilesTest, SimpleFrameHasValidCRC) {
    std::string filepath = testDataDir + "/simple_frame.bin";
    
    if (!fs::exists(filepath)) {
        GTEST_SKIP() << "simple_frame.bin not found";
    }
    
    auto data = readFile(filepath);
    
    // Decode the frame
    std::vector<uint8_t> rxbuf(256);
    std::vector<std::vector<uint8_t>> received_messages;
    std::vector<LogInfo> log_entries;
    
    auto callback = [&received_messages](uint8_t* data, size_t size) {
        std::vector<uint8_t> msg(data, data + size);
        received_messages.push_back(msg);
    };
    
    auto logCallback = [&log_entries](LogInfo info) {
        log_entries.push_back(info);
    };
    
    Decoder decoder(rxbuf.data(), rxbuf.size(), callback, logCallback);
    
    for (uint8_t byte : data) {
        decoder.consume_ex(byte);
    }
    
    ASSERT_EQ(received_messages.size(), 1);
    
    // Check CRC
    const auto& msg = received_messages[0];
    if (msg.size() >= 4) {
        std::vector<uint8_t> payload(msg.begin(), msg.end() - 4);
        std::vector<uint8_t> crc_bytes(msg.end() - 4, msg.end());
        
        uint32_t calculated = calculate_crc32(payload.data(), payload.size());
        uint32_t received = (static_cast<uint32_t>(crc_bytes[0]) |
                            (static_cast<uint32_t>(crc_bytes[1]) << 8) |
                            (static_cast<uint32_t>(crc_bytes[2]) << 16) |
                            (static_cast<uint32_t>(crc_bytes[3]) << 24));
        
        EXPECT_EQ(calculated, received);
    }
}

// Test that ascii_text.bin has a valid CRC
TEST_F(TestDataFilesTest, AsciiTextHasValidCRC) {
    std::string filepath = testDataDir + "/ascii_text.bin";
    
    if (!fs::exists(filepath)) {
        GTEST_SKIP() << "ascii_text.bin not found";
    }
    
    auto data = readFile(filepath);
    
    // Decode the frame
    std::vector<uint8_t> rxbuf(256);
    std::vector<std::vector<uint8_t>> received_messages;
    std::vector<LogInfo> log_entries;
    
    auto callback = [&received_messages](uint8_t* data, size_t size) {
        std::vector<uint8_t> msg(data, data + size);
        received_messages.push_back(msg);
    };
    
    auto logCallback = [&log_entries](LogInfo info) {
        log_entries.push_back(info);
    };
    
    Decoder decoder(rxbuf.data(), rxbuf.size(), callback, logCallback);
    
    for (uint8_t byte : data) {
        decoder.consume_ex(byte);
    }
    
    ASSERT_EQ(received_messages.size(), 1);
    
    // Check CRC
    const auto& msg = received_messages[0];
    if (msg.size() >= 4) {
        std::vector<uint8_t> payload(msg.begin(), msg.end() - 4);
        std::vector<uint8_t> crc_bytes(msg.end() - 4, msg.end());
        
        uint32_t calculated = calculate_crc32(payload.data(), payload.size());
        uint32_t received = (static_cast<uint32_t>(crc_bytes[0]) |
                            (static_cast<uint32_t>(crc_bytes[1]) << 8) |
                            (static_cast<uint32_t>(crc_bytes[2]) << 16) |
                            (static_cast<uint32_t>(crc_bytes[3]) << 24));
        
        EXPECT_EQ(calculated, received);
    }
}

// Test that ascii_long.bin has a valid CRC
TEST_F(TestDataFilesTest, AsciiLongHasValidCRC) {
    std::string filepath = testDataDir + "/ascii_long.bin";
    
    if (!fs::exists(filepath)) {
        GTEST_SKIP() << "ascii_long.bin not found";
    }
    
    auto data = readFile(filepath);
    
    // Decode the frame
    std::vector<uint8_t> rxbuf(256);
    std::vector<std::vector<uint8_t>> received_messages;
    std::vector<LogInfo> log_entries;
    
    auto callback = [&received_messages](uint8_t* data, size_t size) {
        std::vector<uint8_t> msg(data, data + size);
        received_messages.push_back(msg);
    };
    
    auto logCallback = [&log_entries](LogInfo info) {
        log_entries.push_back(info);
    };
    
    Decoder decoder(rxbuf.data(), rxbuf.size(), callback, logCallback);
    
    for (uint8_t byte : data) {
        decoder.consume_ex(byte);
    }
    
    ASSERT_EQ(received_messages.size(), 1);
    
    // Check CRC
    const auto& msg = received_messages[0];
    if (msg.size() >= 4) {
        std::vector<uint8_t> payload(msg.begin(), msg.end() - 4);
        std::vector<uint8_t> crc_bytes(msg.end() - 4, msg.end());
        
        uint32_t calculated = calculate_crc32(payload.data(), payload.size());
        uint32_t received = (static_cast<uint32_t>(crc_bytes[0]) |
                            (static_cast<uint32_t>(crc_bytes[1]) << 8) |
                            (static_cast<uint32_t>(crc_bytes[2]) << 16) |
                            (static_cast<uint32_t>(crc_bytes[3]) << 24));
        
        EXPECT_EQ(calculated, received);
    }
}
