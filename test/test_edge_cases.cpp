// Edge case and invalid data tests for SLIPStream
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include "SLIPStream/Buffer.hpp"
#include "SLIPStream/Encoder.hpp"
#include "SLIPStream/Decoder.hpp"
#include "SLIPStream/SLIP.hpp"

using namespace SLIPStream;

class SLIPEdgeCaseTest : public ::testing::Test {
protected:
    std::vector<uint8_t> buffer;
    const size_t BUFFER_SIZE = 512;
    
    void SetUp() override {
        buffer.resize(BUFFER_SIZE);
    }
};

// ============================================================================
// Buffer API Edge Cases
// ============================================================================

TEST_F(SLIPEdgeCaseTest, NullPointerInput) {
    // Test with null pointer but zero length (should be safe)
    size_t len = encoded_length(nullptr, 0);
    EXPECT_EQ(len, 1u);  // Just END byte
}

TEST_F(SLIPEdgeCaseTest, NullPointerOutputWithZeroLength) {
    // Test encode with null output but zero input length
    size_t written = encode_packet(nullptr, 0, nullptr, 0);
    EXPECT_EQ(written, ENCODE_ERROR);
}

TEST_F(SLIPEdgeCaseTest, VeryLargeDataNoSpecialBytes) {
    // Test with large data without special bytes
    std::vector<uint8_t> large_data(1000, 0x42);
    size_t len = encoded_length(large_data.data(), large_data.size());
    EXPECT_EQ(len, 1001u);  // 1000 + END
}

TEST_F(SLIPEdgeCaseTest, VeryLargeDataAllSpecialBytes) {
    // Test with large data all being special bytes (worst case expansion)
    std::vector<uint8_t> large_data(100, END);
    size_t len = encoded_length(large_data.data(), large_data.size());
    EXPECT_EQ(len, 201u);  // Each END becomes 2 bytes + END = 100*2 + 1
}

TEST_F(SLIPEdgeCaseTest, AlternatingSpecialBytes) {
    // Test with alternating END and ESC bytes
    std::vector<uint8_t> data;
    for (int i = 0; i < 50; i++) {
        data.push_back(END);
        data.push_back(ESC);
    }
    size_t len = encoded_length(data.data(), data.size());
    EXPECT_EQ(len, data.size() * 2 + 1);  // Each becomes 2 bytes + END
}

TEST_F(SLIPEdgeCaseTest, AllByteValuesRoundTrip) {
    // Test round-trip with all possible byte values
    std::vector<uint8_t> all_bytes(256);
    for (int i = 0; i < 256; i++) {
        all_bytes[i] = static_cast<uint8_t>(i);
    }
    
    size_t enc_len = encoded_length(all_bytes.data(), all_bytes.size());
    std::vector<uint8_t> encoded(enc_len);
    size_t written = encode_packet(all_bytes.data(), all_bytes.size(), encoded.data(), encoded.size());
    ASSERT_NE(written, ENCODE_ERROR);
    
    size_t dec_len = decoded_length(encoded.data(), encoded.size());
    ASSERT_NE(dec_len, DECODE_ERROR);
    std::vector<uint8_t> decoded(dec_len);
    size_t dec_written = decode_packet(encoded.data(), encoded.size(), decoded.data(), decoded.size());
    ASSERT_NE(dec_written, DECODE_ERROR);
    
    EXPECT_EQ(all_bytes, decoded);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithMultipleENDMarkers) {
    // Test decoding with multiple END markers (should stop at first)
    const uint8_t input[] = {0x01, 0x02, END, 0x03, 0x04, END};
    size_t len = decoded_length(input, sizeof(input));
    EXPECT_EQ(len, 2u);  // Should stop at first END
    
    std::vector<uint8_t> decoded(len);
    size_t written = decode_packet(input, sizeof(input), decoded.data(), decoded.size());
    EXPECT_EQ(written, 2u);
    EXPECT_EQ(decoded[0], 0x01);
    EXPECT_EQ(decoded[1], 0x02);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithEmbeddedENDInEscape) {
    // Test invalid escape sequence: ESC followed by END
    const uint8_t input[] = {ESC, END, 0x02, END};
    size_t len = decoded_length(input, sizeof(input));
    EXPECT_EQ(len, DECODE_ERROR);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithESCFollowedByESC) {
    // Test invalid escape: ESC followed by ESC (should be ESCESC)
    const uint8_t input[] = {ESC, ESC, 0x02, END};
    size_t len = decoded_length(input, sizeof(input));
    EXPECT_EQ(len, DECODE_ERROR);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithPartialEscapeAtEnd) {
    // Test truncated escape at end of input
    const uint8_t input[] = {0x01, 0x02, ESC};
    size_t len = decoded_length(input, sizeof(input));
    EXPECT_EQ(len, DECODE_ERROR);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithConsecutiveEscapes) {
    // Test multiple consecutive escape sequences
    const uint8_t input[] = {ESC, ESCEND, ESC, ESCESC, ESC, ESCEND, END};
    size_t len = decoded_length(input, sizeof(input));
    EXPECT_EQ(len, 3u);  // END, ESC, END
    
    std::vector<uint8_t> decoded(len);
    size_t written = decode_packet(input, sizeof(input), decoded.data(), decoded.size());
    EXPECT_EQ(written, 3u);
    EXPECT_EQ(decoded[0], END);
    EXPECT_EQ(decoded[1], ESC);
    EXPECT_EQ(decoded[2], END);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithZeroLengthInput) {
    // Test decode with zero length input
    size_t len = decoded_length(nullptr, 0);
    EXPECT_EQ(len, DECODE_ERROR);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithJustENDMarker) {
    // Test decode with just an END marker (empty message)
    const uint8_t input[] = {END};
    size_t len = decoded_length(input, sizeof(input));
    EXPECT_EQ(len, 0u);
    
    std::vector<uint8_t> decoded(1);
    size_t written = decode_packet(input, sizeof(input), decoded.data(), decoded.size());
    EXPECT_EQ(written, 0u);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithJustEscapeSequence) {
    // Test decode with just an escape sequence
    const uint8_t input[] = {ESC, ESCEND, END};
    size_t len = decoded_length(input, sizeof(input));
    EXPECT_EQ(len, 1u);  // Just the decoded END
    
    std::vector<uint8_t> decoded(len);
    size_t written = decode_packet(input, sizeof(input), decoded.data(), decoded.size());
    EXPECT_EQ(written, 1u);
    EXPECT_EQ(decoded[0], END);
}

// ============================================================================
// Encoder Edge Cases
// ============================================================================

struct TestSink {
    std::vector<uint8_t> out;
    WriteStatus status = WriteStatus::Ok;
    
    WriteStatus operator()(uint8_t b) {
        out.push_back(b);
        return status;
    }
    
    void reset() { out.clear(); status = WriteStatus::Ok; }
};

TEST(SLIPEncoderEdge, EmptyPacket) {
    TestSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);
    
    auto [status, consumed] = enc.pushPacket(nullptr, 0);
    EXPECT_EQ(status, WriteStatus::Ok);
    EXPECT_EQ(consumed, 0u);
    EXPECT_EQ(sink.out.size(), 1u);  // Just END
    EXPECT_EQ(sink.out[0], END);
}

TEST(SLIPEncoderEdge, SingleBytePacket) {
    TestSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);
    
    const uint8_t data[] = {0x42};
    auto [status, consumed] = enc.pushPacket(data, sizeof(data));
    EXPECT_EQ(status, WriteStatus::Ok);
    EXPECT_EQ(consumed, 1u);
    EXPECT_EQ(sink.out.size(), 2u);  // 0x42 + END
}

TEST(SLIPEncoderEdge, PacketWithOnlySpecialBytes) {
    TestSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);
    
    const uint8_t data[] = {END, ESC, END, ESC};
    auto [status, consumed] = enc.pushPacket(data, sizeof(data));
    EXPECT_EQ(status, WriteStatus::Ok);
    EXPECT_EQ(consumed, 4u);
    // Each special byte becomes 2 bytes + END = 4*2 + 1 = 9
    EXPECT_EQ(sink.out.size(), 9u);
}

TEST(SLIPEncoderEdge, VerySmallBuffer) {
    TestSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 4, 2);
    
    const uint8_t data[] = {0x01, 0x02};
    auto [status, consumed] = enc.pushPacket(data, sizeof(data));
    // Should either succeed or retry based on buffer size
    EXPECT_TRUE(status == WriteStatus::Ok || status == WriteStatus::RetryLater);
}

TEST(SLIPEncoderEdge, ZeroMaxSendChunk) {
    TestSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 0);
    
    const uint8_t data[] = {0x01, 0x02};
    auto [status, consumed] = enc.pushPacket(data, sizeof(data));
    // With zero chunk size, should still work but might not flush immediately
    EXPECT_TRUE(status == WriteStatus::Ok || status == WriteStatus::RetryLater);
}

TEST(SLIPEncoderEdge, MultipleFlushes) {
    TestSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);
    
    // Push a packet
    const uint8_t data[] = {0x01, 0x02};
    enc.pushPacket(data, sizeof(data));
    
    // Multiple flushes should be safe
    WriteStatus status1 = enc.flush();
    WriteStatus status2 = enc.flush();
    EXPECT_EQ(status1, WriteStatus::Ok);
    EXPECT_EQ(status2, WriteStatus::Ok);
}

// ============================================================================
// Decoder Edge Cases
// ============================================================================

struct MessageCapture {
    std::vector<std::vector<uint8_t>> messages;
    
    void callback(uint8_t* data, size_t size) {
        std::vector<uint8_t> msg(data, data + size);
        messages.push_back(msg);
    }
    
    std::function<void(uint8_t*, size_t)> getFn() {
        return [this](uint8_t* data, size_t size) { callback(data, size); };
    }
};

TEST(SLIPDecoderEdge, ZeroSizeBuffer) {
    std::vector<uint8_t> rxbuf(1);  // Very small buffer
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Try to consume data that would overflow
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    for (size_t i = 0; i < sizeof(data); i++) {
        dec.consume(data[i]);
    }
    
    // Should handle gracefully (may reset or log error)
}

TEST(SLIPDecoderEdge, ConsecutiveENDMarkers) {
    std::vector<uint8_t> rxbuf(256);
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    const uint8_t data[] = {END, END, END};
    for (size_t i = 0; i < sizeof(data); i++) {
        dec.consume(data[i]);
    }
    
    // Should receive three empty messages
    EXPECT_EQ(capture.messages.size(), 3u);
    for (const auto& msg : capture.messages) {
        EXPECT_EQ(msg.size(), 0u);
    }
}

TEST(SLIPDecoderEdge, EmptyMessage) {
    std::vector<uint8_t> rxbuf(256);
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    dec.consume(END);
    
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 0u);
}

TEST(SLIPDecoderEdge, MessageWithOnlyEscapeSequences) {
    std::vector<uint8_t> rxbuf(256);
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    const uint8_t data[] = {ESC, ESCEND, ESC, ESCESC, END};
    for (size_t i = 0; i < sizeof(data); i++) {
        dec.consume(data[i]);
    }
    
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 2u);
    EXPECT_EQ(capture.messages[0][0], END);
    EXPECT_EQ(capture.messages[0][1], ESC);
}

TEST(SLIPDecoderEdge, ResetMidMessage) {
    std::vector<uint8_t> rxbuf(256);
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Start a message
    dec.consume(0x01);
    dec.consume(0x02);
    
    // Reset before END
    dec.reset();
    
    // Start new message
    const uint8_t data[] = {0x03, 0x04, END};
    for (size_t i = 0; i < sizeof(data); i++) {
        dec.consume(data[i]);
    }
    
    // Should only receive the new message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 2u);
    EXPECT_EQ(capture.messages[0][0], 0x03);
    EXPECT_EQ(capture.messages[0][1], 0x04);
}

TEST(SLIPDecoderEdge, ResetAfterEscape) {
    std::vector<uint8_t> rxbuf(256);
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Start escape sequence
    dec.consume(ESC);
    
    // Reset before completing escape
    dec.reset();
    
    // Send valid message
    const uint8_t data[] = {0x01, END};
    for (size_t i = 0; i < sizeof(data); i++) {
        dec.consume(data[i]);
    }
    
    // Should receive valid message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 1u);
    EXPECT_EQ(capture.messages[0][0], 0x01);
}

TEST(SLIPDecoderEdge, LargeMessage) {
    std::vector<uint8_t> rxbuf(512);
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    std::vector<uint8_t> large_data(300, 0x42);
    large_data.push_back(END);
    
    for (size_t i = 0; i < large_data.size(); i++) {
        dec.consume(large_data[i]);
    }
    
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 300u);
}

TEST(SLIPDecoderEdge, MaximumBufferSizeMessage) {
    std::vector<uint8_t> rxbuf(256);
    MessageCapture capture;
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Fill buffer to near capacity
    std::vector<uint8_t> data(254, 0x42);  // Leave space for potential escapes
    data.push_back(END);
    
    for (size_t i = 0; i < data.size(); i++) {
        dec.consume(data[i]);
    }
    
    // Should handle gracefully
    EXPECT_GE(capture.messages.size(), 1u);
}

// ============================================================================
// Invalid Data Tests
// ============================================================================

TEST_F(SLIPEdgeCaseTest, InvalidEscapeAfterESC) {
    // Test all possible invalid bytes after ESC
    for (int i = 0; i < 256; i++) {
        uint8_t byte = static_cast<uint8_t>(i);
        if (byte == ESCEND || byte == ESCESC) continue;  // Valid
        
        const uint8_t input[] = {ESC, byte, END};
        size_t len = decoded_length(input, sizeof(input));
        EXPECT_EQ(len, DECODE_ERROR) << "Failed for byte: " << i;
    }
}

TEST_F(SLIPEdgeCaseTest, DecodeWithCorruptedEscape) {
    // Test corrupted escape sequences
    const uint8_t test_cases[][5] = {
        {ESC, 0x00, END},  // ESC followed by 0x00
        {ESC, 0xFF, END},  // ESC followed by 0xFF
        {ESC, 0x01, END},  // ESC followed by 0x01
        {ESC, 0xFE, END},  // ESC followed by 0xFE
    };
    
    for (const auto& test : test_cases) {
        size_t len = decoded_length(test, 3);
        EXPECT_EQ(len, DECODE_ERROR);
    }
}

TEST_F(SLIPEdgeCaseTest, EncodeWithOutputBufferExactlyMinimum) {
    // Test with output buffer exactly at minimum required size
    const uint8_t input[] = {0x01, 0x02};
    size_t needed = encoded_length(input, sizeof(input));
    
    std::vector<uint8_t> output(needed);
    size_t written = encode_packet(input, sizeof(input), output.data(), output.size());
    EXPECT_EQ(written, needed);
}

TEST_F(SLIPEdgeCaseTest, DecodeWithOutputBufferExactlyMinimum) {
    // Test with output buffer exactly at minimum required size
    const uint8_t input[] = {0x01, 0x02, END};
    size_t needed = decoded_length(input, sizeof(input));
    
    std::vector<uint8_t> output(needed);
    size_t written = decode_packet(input, sizeof(input), output.data(), output.size());
    EXPECT_EQ(written, needed);
}

TEST_F(SLIPEdgeCaseTest, BufferOneByteTooSmall) {
    // Test with buffer exactly one byte too small
    const uint8_t input[] = {0x01, 0x02, 0x03};
    size_t needed = encoded_length(input, sizeof(input));
    
    std::vector<uint8_t> output(needed - 1);
    size_t written = encode_packet(input, sizeof(input), output.data(), output.size());
    EXPECT_EQ(written, ENCODE_ERROR);
}

TEST_F(SLIPEdgeCaseTest, DecodeBufferOneByteTooSmall) {
    // Test with decode buffer exactly one byte too small
    const uint8_t input[] = {0x01, 0x02, END};
    size_t needed = decoded_length(input, sizeof(input));
    
    std::vector<uint8_t> output(needed - 1);
    size_t written = decode_packet(input, sizeof(input), output.data(), output.size());
    EXPECT_EQ(written, DECODE_ERROR);
}
