// Comprehensive tests for Buffer API with enhanced error reporting
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include "SLIPStream/Buffer.hpp"

using namespace SLIPStream;

class SLIPBufferEnhancedTest : public ::testing::Test {
protected:
    std::vector<uint8_t> buffer;
    const size_t BUFFER_SIZE = 256;
    
    void SetUp() override {
        buffer.resize(BUFFER_SIZE);
    }
};

// ============================================================================
// Enhanced Encoding Tests
// ============================================================================

TEST_F(SLIPBufferEnhancedTest, EncodedLengthExSuccess) {
    const uint8_t input[] = {0x01, 0x02, 0x03};
    Result<size_t> result = encoded_length_ex(input, sizeof(input));
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 4u);  // 3 bytes + END
}

TEST_F(SLIPBufferEnhancedTest, EncodedLengthExWithEscapes) {
    const uint8_t input[] = {END, ESC, 0x55};
    Result<size_t> result = encoded_length_ex(input, sizeof(input));
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 6u);  // Each special byte becomes 2 + END
}

TEST_F(SLIPBufferEnhancedTest, EncodedLengthExEmpty) {
    Result<size_t> result = encoded_length_ex(nullptr, 0);
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 1u);  // Just END
}

TEST_F(SLIPBufferEnhancedTest, EncodePacketExSuccess) {
    const uint8_t input[] = {0x01, 0x02, 0x03};
    Result<size_t> result = encode_packet_ex(input, sizeof(input), buffer.data(), buffer.size());
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 4u);
    EXPECT_EQ(buffer[0], 0x01);
    EXPECT_EQ(buffer[1], 0x02);
    EXPECT_EQ(buffer[2], 0x03);
    EXPECT_EQ(buffer[3], END);
}

TEST_F(SLIPBufferEnhancedTest, EncodePacketExBufferTooSmall) {
    const uint8_t input[] = {0x01, 0x02, 0x03};
    Result<size_t> result = encode_packet_ex(input, sizeof(input), buffer.data(), 2);
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::EncodeBufferTooSmall);
    EXPECT_EQ(result.error.position, 0u);
    EXPECT_STREQ(result.error.message, "Output buffer too small");
}

TEST_F(SLIPBufferEnhancedTest, EncodePacketExWithEscapes) {
    const uint8_t input[] = {END, ESC, 0x55};
    Result<size_t> result = encode_packet_ex(input, sizeof(input), buffer.data(), buffer.size());
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 6u);
    EXPECT_EQ(buffer[0], ESC);
    EXPECT_EQ(buffer[1], ESCEND);
    EXPECT_EQ(buffer[2], ESC);
    EXPECT_EQ(buffer[3], ESCESC);
    EXPECT_EQ(buffer[4], 0x55);
    EXPECT_EQ(buffer[5], END);
}

TEST_F(SLIPBufferEnhancedTest, EncodePacketExEscapedENDOverflow) {
    const uint8_t input[] = {END, 0x02, 0x03};
    Result<size_t> result = encode_packet_ex(input, sizeof(input), buffer.data(), 3);
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::EncodeBufferTooSmall);
}

TEST_F(SLIPBufferEnhancedTest, EncodePacketExEscapedESCOverflow) {
    const uint8_t input[] = {ESC, 0x02, 0x03};
    Result<size_t> result = encode_packet_ex(input, sizeof(input), buffer.data(), 3);
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::EncodeBufferTooSmall);
}

TEST_F(SLIPBufferEnhancedTest, EncodePacketExDataByteOverflow) {
    const uint8_t input[] = {0x01, 0x02, 0x03};
    Result<size_t> result = encode_packet_ex(input, sizeof(input), buffer.data(), 3);
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::EncodeBufferTooSmall);
    EXPECT_EQ(result.error.position, 2u);
    EXPECT_TRUE(strcmp(result.error.message, "Output buffer too small") == 0 || 
                strcmp(result.error.message, "Output buffer too small for data byte") == 0);
}

TEST_F(SLIPBufferEnhancedTest, EncodePacketExENDByteOverflow) {
    const uint8_t input[] = {0x01};
    Result<size_t> result = encode_packet_ex(input, sizeof(input), buffer.data(), 1);
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::EncodeBufferTooSmall);
}

// ============================================================================
// Enhanced Decoding Tests
// ============================================================================

TEST_F(SLIPBufferEnhancedTest, DecodedLengthExSuccess) {
    const uint8_t input[] = {0x01, 0x02, 0x03, END};
    Result<size_t> result = decoded_length_ex(input, sizeof(input));
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 3u);
}

TEST_F(SLIPBufferEnhancedTest, DecodedLengthExWithEscapes) {
    const uint8_t input[] = {ESC, ESCEND, ESC, ESCESC, 0x05, END};
    Result<size_t> result = decoded_length_ex(input, sizeof(input));
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 3u);
}

TEST_F(SLIPBufferEnhancedTest, DecodedLengthExNoENDMarker) {
    const uint8_t input[] = {0x01, 0x02, 0x03};
    Result<size_t> result = decoded_length_ex(input, sizeof(input));
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeNoEndMarker);
    EXPECT_EQ(result.error.position, 3u);
    EXPECT_STREQ(result.error.message, "No END marker (0xC0) found in input data");
}

TEST_F(SLIPBufferEnhancedTest, DecodedLengthExTruncatedEscape) {
    const uint8_t input[] = {ESC};
    Result<size_t> result = decoded_length_ex(input, sizeof(input));
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeTruncatedEscape);
    EXPECT_EQ(result.error.position, 0u);
    EXPECT_STREQ(result.error.message, "Truncated escape sequence at end of input");
}

TEST_F(SLIPBufferEnhancedTest, DecodedLengthExInvalidEscape) {
    const uint8_t input[] = {ESC, 0xFF, END};
    Result<size_t> result = decoded_length_ex(input, sizeof(input));
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeInvalidEscapeSequence);
    EXPECT_EQ(result.error.position, 0u);
    EXPECT_STREQ(result.error.message, "Invalid escape sequence: ESC not followed by ESCEND or ESCESC");
}

TEST_F(SLIPBufferEnhancedTest, DecodePacketExSuccess) {
    const uint8_t input[] = {0x01, 0x02, 0x03, END};
    Result<size_t> result = decode_packet_ex(input, sizeof(input), buffer.data(), buffer.size());
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 3u);
    EXPECT_EQ(buffer[0], 0x01);
    EXPECT_EQ(buffer[1], 0x02);
    EXPECT_EQ(buffer[2], 0x03);
}

TEST_F(SLIPBufferEnhancedTest, DecodePacketExWithEscapes) {
    const uint8_t input[] = {ESC, ESCEND, 0x02, ESC, ESCESC, END};
    Result<size_t> result = decode_packet_ex(input, sizeof(input), buffer.data(), buffer.size());
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.value, 3u);
    EXPECT_EQ(buffer[0], END);
    EXPECT_EQ(buffer[1], 0x02);
    EXPECT_EQ(buffer[2], ESC);
}

TEST_F(SLIPBufferEnhancedTest, DecodePacketExBufferTooSmall) {
    const uint8_t input[] = {0x01, 0x02, 0x03, END};
    Result<size_t> result = decode_packet_ex(input, sizeof(input), buffer.data(), 2);
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeBufferTooSmall);
    EXPECT_EQ(result.error.position, 2u);
    EXPECT_STREQ(result.error.message, "Output buffer too small for data byte");
}

TEST_F(SLIPBufferEnhancedTest, DecodePacketExTruncatedEscape) {
    const uint8_t input[] = {ESC};
    Result<size_t> result = decode_packet_ex(input, sizeof(input), buffer.data(), buffer.size());
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeTruncatedEscape);
    EXPECT_EQ(result.error.position, 0u);
    EXPECT_STREQ(result.error.message, "Truncated escape sequence at end of input");
}

TEST_F(SLIPBufferEnhancedTest, DecodePacketExInvalidEscape) {
    const uint8_t input[] = {ESC, 0x55, END};
    Result<size_t> result = decode_packet_ex(input, sizeof(input), buffer.data(), buffer.size());
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeInvalidEscapeSequence);
    EXPECT_EQ(result.error.position, 0u);
    EXPECT_STREQ(result.error.message, "Invalid escape sequence: ESC not followed by ESCEND or ESCESC");
}

TEST_F(SLIPBufferEnhancedTest, DecodePacketExNoEND) {
    const uint8_t input[] = {0x01, 0x02, 0x03};
    Result<size_t> result = decode_packet_ex(input, sizeof(input), buffer.data(), buffer.size());
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeNoEndMarker);
    EXPECT_EQ(result.error.position, 3u);
    EXPECT_STREQ(result.error.message, "No END terminator found in input data");
}

TEST_F(SLIPBufferEnhancedTest, DecodePacketExDecodedDataOverflow) {
    const uint8_t input[] = {ESC, ESCEND, 0x02, END};
    Result<size_t> result = decode_packet_ex(input, sizeof(input), buffer.data(), 1);
    
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::DecodeBufferTooSmall);
    EXPECT_EQ(result.error.position, 1u);
    EXPECT_TRUE(strcmp(result.error.message, "Output buffer too small for data byte") == 0 || 
                strcmp(result.error.message, "Output buffer too small for decoded data") == 0);
}

// ============================================================================
// Round-trip tests with enhanced error reporting
// ============================================================================

TEST_F(SLIPBufferEnhancedTest, RoundTripExSimple) {
    const uint8_t original[] = {0x12, 0x34, 0x56, 0x78};
    std::vector<uint8_t> encoded(256);
    std::vector<uint8_t> decoded(256);
    
    Result<size_t> enc_result = encode_packet_ex(original, sizeof(original), encoded.data(), encoded.size());
    ASSERT_TRUE(enc_result.is_success());
    
    Result<size_t> dec_result = decode_packet_ex(encoded.data(), enc_result.value, decoded.data(), decoded.size());
    ASSERT_TRUE(dec_result.is_success());
    
    EXPECT_EQ(dec_result.value, sizeof(original));
    EXPECT_EQ(0, std::memcmp(original, decoded.data(), sizeof(original)));
}

TEST_F(SLIPBufferEnhancedTest, RoundTripExComplex) {
    const uint8_t original[] = {
        0x00, END, 0x01, ESC, 0x02,
        END, END, ESC, ESC,
        0xFF, 0xAA, 0x55, 0x00
    };
    std::vector<uint8_t> encoded(256);
    std::vector<uint8_t> decoded(256);
    
    Result<size_t> enc_result = encode_packet_ex(original, sizeof(original), encoded.data(), encoded.size());
    ASSERT_TRUE(enc_result.is_success());
    
    Result<size_t> dec_result = decode_packet_ex(encoded.data(), enc_result.value, decoded.data(), decoded.size());
    ASSERT_TRUE(dec_result.is_success());
    
    EXPECT_EQ(dec_result.value, sizeof(original));
    EXPECT_EQ(0, std::memcmp(original, decoded.data(), sizeof(original)));
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_F(SLIPBufferEnhancedTest, LargeDataWithNoSpecialBytes) {
    std::vector<uint8_t> large_data(200, 0x42);
    std::vector<uint8_t> encoded(400);
    std::vector<uint8_t> decoded(400);
    
    Result<size_t> enc_result = encode_packet_ex(large_data.data(), large_data.size(), encoded.data(), encoded.size());
    ASSERT_TRUE(enc_result.is_success());
    EXPECT_EQ(enc_result.value, 201u);  // 200 bytes + END
    
    Result<size_t> dec_result = decode_packet_ex(encoded.data(), enc_result.value, decoded.data(), decoded.size());
    ASSERT_TRUE(dec_result.is_success());
    EXPECT_EQ(dec_result.value, 200u);
}

TEST_F(SLIPBufferEnhancedTest, AllSpecialBytes) {
    const uint8_t special[] = {END, ESC, END, ESC, END, ESC};
    std::vector<uint8_t> encoded(50);
    std::vector<uint8_t> decoded(50);
    
    Result<size_t> enc_result = encode_packet_ex(special, sizeof(special), encoded.data(), encoded.size());
    ASSERT_TRUE(enc_result.is_success());
    
    Result<size_t> dec_result = decode_packet_ex(encoded.data(), enc_result.value, decoded.data(), decoded.size());
    ASSERT_TRUE(dec_result.is_success());
    
    EXPECT_EQ(dec_result.value, sizeof(special));
    EXPECT_EQ(0, std::memcmp(special, decoded.data(), sizeof(special)));
}
