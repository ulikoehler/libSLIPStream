// CRC32 tests for SLIPStream
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include "SLIPStream/CRC32.hpp"

using namespace SLIPStream;

class CRC32Test : public ::testing::Test {
protected:
    std::vector<uint8_t> buffer;
    const size_t BUFFER_SIZE = 512;
    
    void SetUp() override {
        buffer.resize(BUFFER_SIZE);
    }
};

// ============================================================================
// Basic CRC32 Calculation Tests
// ============================================================================

TEST_F(CRC32Test, EmptyData) {
    const uint8_t empty[] = {};
    uint32_t crc = calculate_crc32(empty, 0);
    // CRC32 of empty data should be 0xFFFFFFFF (initial value)
    EXPECT_EQ(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, SingleByteZero) {
    const uint8_t data[] = {0x00};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, SingleByteOne) {
    const uint8_t data[] = {0x01};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, SimpleString) {
    const uint8_t data[] = {'H', 'e', 'l', 'l', 'o'};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, AllZeros) {
    std::vector<uint8_t> data(256, 0x00);
    uint32_t crc = calculate_crc32(data.data(), data.size());
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, AllOnes) {
    std::vector<uint8_t> data(256, 0xFF);
    uint32_t crc = calculate_crc32(data.data(), data.size());
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

// ============================================================================
// CRC32 with Initial Value Tests
// ============================================================================

TEST_F(CRC32Test, InitialValueDefault) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    uint32_t crc1 = calculate_crc32(data, sizeof(data));
    uint32_t crc2 = calculate_crc32_with_initial(data, sizeof(data), 0xFFFFFFFF);
    EXPECT_EQ(crc1, crc2);
}

TEST_F(CRC32Test, InitialValueZero) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    uint32_t crc = calculate_crc32_with_initial(data, sizeof(data), 0x00000000);
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, IncrementalCalculation) {
    const uint8_t data1[] = {0x01, 0x02};
    const uint8_t data2[] = {0x03, 0x04};
    
    uint32_t crc1 = calculate_crc32(data1, sizeof(data1));
    uint32_t crc_incremental = calculate_crc32_with_initial(data2, sizeof(data2), crc1);
    
    const uint8_t combined[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t crc_combined = calculate_crc32(combined, sizeof(combined));
    
    EXPECT_EQ(crc_incremental, crc_combined);
}

// ============================================================================
// Append CRC32 Tests
// ============================================================================

TEST_F(CRC32Test, AppendCRC32) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    buffer[0] = data[0];
    buffer[1] = data[1];
    buffer[2] = data[2];
    
    size_t new_length = append_crc32(buffer.data(), 3);
    EXPECT_EQ(new_length, 7u);  // 3 data + 4 CRC bytes
    
    // Verify CRC is appended
    uint32_t original_crc = calculate_crc32(data, sizeof(data));
    uint32_t extracted_crc;
    extract_crc32(buffer.data(), new_length, &extracted_crc);
    
    EXPECT_EQ(extracted_crc, original_crc);
}

TEST_F(CRC32Test, AppendCRC32NullPointer) {
    size_t length = append_crc32(nullptr, 10);
    EXPECT_EQ(length, 10u);  // Should return original length unchanged
}

TEST_F(CRC32Test, AppendCRC32ZeroLength) {
    buffer.resize(4);
    size_t new_length = append_crc32(buffer.data(), 0);
    EXPECT_EQ(new_length, 4u);
    
    // CRC of empty data should be 0xFFFFFFFF
    uint32_t extracted_crc;
    extract_crc32(buffer.data(), new_length, &extracted_crc);
    EXPECT_EQ(extracted_crc, 0xFFFFFFFFu);
}

// ============================================================================
// Extract CRC32 Tests
// ============================================================================

TEST_F(CRC32Test, ExtractCRC32) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    
    // Manually append CRC in little-endian format
    buffer[0] = data[0];
    buffer[1] = data[1];
    buffer[2] = data[2];
    buffer[3] = crc & 0xFF;
    buffer[4] = (crc >> 8) & 0xFF;
    buffer[5] = (crc >> 16) & 0xFF;
    buffer[6] = (crc >> 24) & 0xFF;
    
    uint32_t extracted_crc;
    size_t data_length = extract_crc32(buffer.data(), 7, &extracted_crc);
    
    EXPECT_EQ(data_length, 3u);
    EXPECT_EQ(extracted_crc, crc);
}

TEST_F(CRC32Test, ExtractCRC32NullPointer) {
    uint32_t crc;
    size_t length = extract_crc32(nullptr, 10, &crc);
    EXPECT_EQ(length, 0u);
}

TEST_F(CRC32Test, ExtractCRC32TooShort) {
    uint32_t crc;
    size_t length = extract_crc32(buffer.data(), 3, &crc);
    EXPECT_EQ(length, 0u);
}

TEST_F(CRC32Test, ExtractCRC32NullOutput) {
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    size_t length = extract_crc32(data, sizeof(data), nullptr);
    EXPECT_EQ(length, 0u);
}

// ============================================================================
// Verify CRC32 Tests
// ============================================================================

TEST_F(CRC32Test, VerifyCRC32Valid) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    
    // Manually append CRC in little-endian format
    buffer[0] = data[0];
    buffer[1] = data[1];
    buffer[2] = data[2];
    buffer[3] = crc & 0xFF;
    buffer[4] = (crc >> 8) & 0xFF;
    buffer[5] = (crc >> 16) & 0xFF;
    buffer[6] = (crc >> 24) & 0xFF;
    
    bool valid = verify_crc32(buffer.data(), 7);
    EXPECT_TRUE(valid);
}

TEST_F(CRC32Test, VerifyCRC32Invalid) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    
    // Append invalid CRC
    buffer[0] = data[0];
    buffer[1] = data[1];
    buffer[2] = data[2];
    buffer[3] = 0x00;  // Wrong CRC
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    
    bool valid = verify_crc32(buffer.data(), 7);
    EXPECT_FALSE(valid);
}

TEST_F(CRC32Test, VerifyCRC32NullPointer) {
    bool valid = verify_crc32(nullptr, 10);
    EXPECT_FALSE(valid);
}

TEST_F(CRC32Test, VerifyCRC32TooShort) {
    const uint8_t data[] = {0x01, 0x02};
    bool valid = verify_crc32(data, sizeof(data));
    EXPECT_FALSE(valid);
}

// ============================================================================
// Known Value Tests (for Python/C++ parity verification)
// ============================================================================

TEST_F(CRC32Test, KnownValueHelloWorld) {
    const uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    // Python: calculate_crc32(b"Hello, World!") = 0x8d87dbf0
    EXPECT_EQ(crc, 0x8d87dbf0u);
}

TEST_F(CRC32Test, KnownValueTestData) {
    const uint8_t data[] = {'T', 'e', 's', 't', ' ', 'D', 'a', 't', 'a'};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    // Python: calculate_crc32(b"Test Data") = 0x228d74ca
    EXPECT_EQ(crc, 0x228d74cau);
}

TEST_F(CRC32Test, KnownValue123456789) {
    const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    // Python: calculate_crc32(b"123456789") = 0xe73d231e
    EXPECT_EQ(crc, 0xe73d231eu);
}

TEST_F(CRC32Test, KnownValueEmpty) {
    const uint8_t data[] = {};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    // Python: calculate_crc32(b"") = 0xFFFFFFFF
    EXPECT_EQ(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, KnownValueSingleByteA) {
    const uint8_t data[] = {'A'};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    // Python: calculate_crc32(b"A") = 0xC01105E2
    EXPECT_EQ(crc, 0xC01105E2u);
}

TEST_F(CRC32Test, KnownValueAllBytes) {
    std::vector<uint8_t> data(256);
    for (int i = 0; i < 256; i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    uint32_t crc = calculate_crc32(data.data(), data.size());
    // Python: calculate_crc32(bytes(range(256))) = 0x0DC76714
    EXPECT_EQ(crc, 0x0DC76714u);
}

TEST_F(CRC32Test, KnownValueABCD) {
    const uint8_t data[] = {'A', 'B', 'C', 'D'};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    // Python: calculate_crc32(b"ABCD") = 0x02975E34
    EXPECT_EQ(crc, 0x02975E34u);
}

TEST_F(CRC32Test, KnownValue12345) {
    const uint8_t data[] = {'1', '2', '3', '4', '5'};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    // Python: calculate_crc32(b"12345") = 0x8F7F2172
    EXPECT_EQ(crc, 0x8F7F2172u);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(CRC32Test, LargeData) {
    // Test with 1KB of data
    std::vector<uint8_t> large_data(1024);
    for (size_t i = 0; i < large_data.size(); i++) {
        large_data[i] = static_cast<uint8_t>(i % 256);
    }
    
    uint32_t crc = calculate_crc32(large_data.data(), large_data.size());
    EXPECT_NE(crc, 0xFFFFFFFFu);
    EXPECT_NE(crc, 0x00000000u);
}

TEST_F(CRC32Test, AlternatingPattern) {
    std::vector<uint8_t> data(256);
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = (i % 2 == 0) ? 0xAA : 0x55;
    }
    
    uint32_t crc = calculate_crc32(data.data(), data.size());
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, SequentialPattern) {
    std::vector<uint8_t> data(256);
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<uint8_t>(i);
    }
    
    uint32_t crc = calculate_crc32(data.data(), data.size());
    EXPECT_NE(crc, 0xFFFFFFFFu);
}

TEST_F(CRC32Test, SingleByteAllValues) {
    // Test CRC32 of all possible single byte values
    for (int i = 0; i < 256; i++) {
        uint8_t byte = static_cast<uint8_t>(i);
        uint32_t crc = calculate_crc32(&byte, 1);
        EXPECT_NE(crc, 0xFFFFFFFFu);
    }
}

TEST_F(CRC32Test, LittleEndianByteOrder) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    uint32_t crc = calculate_crc32(data, sizeof(data));
    
    buffer[0] = data[0];
    buffer[1] = data[1];
    buffer[2] = data[2];
    size_t new_length = append_crc32(buffer.data(), 3);
    
    // Verify little-endian byte order (LSB first)
    EXPECT_EQ(buffer[3], crc & 0xFF);
    EXPECT_EQ(buffer[4], (crc >> 8) & 0xFF);
    EXPECT_EQ(buffer[5], (crc >> 16) & 0xFF);
    EXPECT_EQ(buffer[6], (crc >> 24) & 0xFF);
}

// ============================================================================
// Round-trip Tests
// ============================================================================

TEST_F(CRC32Test, RoundTripAppendExtract) {
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    memcpy(buffer.data(), data, sizeof(data));
    size_t with_crc = append_crc32(buffer.data(), sizeof(data));
    
    uint32_t extracted_crc;
    size_t without_crc = extract_crc32(buffer.data(), with_crc, &extracted_crc);
    
    EXPECT_EQ(without_crc, sizeof(data));
    EXPECT_EQ(memcmp(buffer.data(), data, sizeof(data)), 0);
    
    uint32_t calculated_crc = calculate_crc32(data, sizeof(data));
    EXPECT_EQ(extracted_crc, calculated_crc);
}

TEST_F(CRC32Test, RoundTripVerify) {
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    memcpy(buffer.data(), data, sizeof(data));
    size_t with_crc = append_crc32(buffer.data(), sizeof(data));
    
    bool valid = verify_crc32(buffer.data(), with_crc);
    EXPECT_TRUE(valid);
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F(CRC32Test, ConsistencySameData) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    
    uint32_t crc1 = calculate_crc32(data, sizeof(data));
    uint32_t crc2 = calculate_crc32(data, sizeof(data));
    uint32_t crc3 = calculate_crc32(data, sizeof(data));
    
    EXPECT_EQ(crc1, crc2);
    EXPECT_EQ(crc2, crc3);
}

TEST_F(CRC32Test, ConsistencyDifferentOrder) {
    const uint8_t data1[] = {0x01, 0x02, 0x03};
    const uint8_t data2[] = {0x03, 0x02, 0x01};
    
    uint32_t crc1 = calculate_crc32(data1, sizeof(data1));
    uint32_t crc2 = calculate_crc32(data2, sizeof(data2));
    
    EXPECT_NE(crc1, crc2);
}
