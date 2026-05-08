// Comprehensive tests for Encoder class with enhanced error reporting
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <deque>
#include <functional>
#include "SLIPStream/Encoder.hpp"
#include "SLIPStream/SLIP.hpp"

using namespace SLIPStream;

// Helper: capture output into a vector, with optional backpressure simulation
struct BackpressuredSink {
    std::vector<uint8_t> out;
    // How many bytes to accept before returning RetryLater for the next call
    size_t acceptThenBlock = SIZE_MAX; // default: never block
    size_t accepted = 0;

    WriteStatus operator()(uint8_t b) {
        if (accepted >= acceptThenBlock) return WriteStatus::RetryLater;
        out.push_back(b);
        accepted++;
        return WriteStatus::Ok;
    }

    void reset() { out.clear(); accepted = 0; }
};

// Helper: sink that returns Error
struct ErrorSink {
    std::vector<uint8_t> out;
    size_t errorAfter = SIZE_MAX;
    size_t accepted = 0;

    WriteStatus operator()(uint8_t b) {
        if (accepted >= errorAfter) return WriteStatus::Error;
        out.push_back(b);
        accepted++;
        return WriteStatus::Ok;
    }

    void reset() { out.clear(); accepted = 0; }
};

class SLIPEncoderEnhancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default setup
    }
};

// ============================================================================
// Enhanced flush tests
// ============================================================================

TEST(SLIPEncoderEnhanced, FlushExSuccess) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);
    
    // Push a packet to have something to flush
    const uint8_t in[] = {0x01, 0x02, 0x03};
    enc.pushPacket(in, sizeof(in));
    
    WriteResult result = enc.flush_ex();
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.status, WriteStatus::Ok);
    EXPECT_EQ(result.error.code, ErrorCode::Success);
}

TEST(SLIPEncoderEnhanced, FlushExRetryLater) {
    BackpressuredSink sink;
    sink.acceptThenBlock = 0; // Block immediately
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 16, 4);
    
    // Push a packet
    const uint8_t in[] = {0x01, 0x02, 0x03};
    enc.pushPacket(in, sizeof(in));
    
    WriteResult result = enc.flush_ex();
    EXPECT_TRUE(result.is_retry());
    EXPECT_EQ(result.status, WriteStatus::RetryLater);
}

TEST(SLIPEncoderEnhanced, FlushExError) {
    ErrorSink sink;
    sink.errorAfter = 0; // Error immediately
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 16, 4);
    
    // Push a packet
    const uint8_t in[] = {0x01, 0x02, 0x03};
    enc.pushPacket(in, sizeof(in));
    
    WriteResult result = enc.flush_ex();
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.status, WriteStatus::Error);
    EXPECT_EQ(result.error.code, ErrorCode::EncodeInternalError);
    EXPECT_STREQ(result.error.message, "Output function returned error");
}

// ============================================================================
// Enhanced pushPacket tests
// ============================================================================

TEST(SLIPEncoderEnhanced, PushPacketExSimpleSuccess) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);

    const uint8_t in[] = {0x01, 0x02, 0x03};
    Encoder::PushPacketResult result = enc.pushPacket_ex(in, sizeof(in));
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.status, WriteStatus::Ok);
    EXPECT_EQ(result.consumed, sizeof(in));
    EXPECT_EQ(result.error.code, ErrorCode::Success);
    
    // Verify output
    ASSERT_EQ(sink.out.size(), sizeof(in) + 1);
    EXPECT_EQ(sink.out[0], 0x01);
    EXPECT_EQ(sink.out[1], 0x02);
    EXPECT_EQ(sink.out[2], 0x03);
    EXPECT_EQ(sink.out[3], END);
}

TEST(SLIPEncoderEnhanced, PushPacketExWithEscapes) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);

    const uint8_t in[] = {END, ESC, 0x55};
    Encoder::PushPacketResult result = enc.pushPacket_ex(in, sizeof(in));
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.status, WriteStatus::Ok);
    EXPECT_EQ(result.consumed, sizeof(in));
    
    // Verify escaped output
    std::vector<uint8_t> expected = {ESC, ESCEND, ESC, ESCESC, 0x55, END};
    EXPECT_EQ(sink.out.size(), expected.size());
    EXPECT_EQ(0, std::memcmp(sink.out.data(), expected.data(), expected.size()));
}

TEST(SLIPEncoderEnhanced, PushPacketExRetryLater) {
    BackpressuredSink sink;
    sink.acceptThenBlock = 2; // Accept 2 bytes then block
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 16, 4);

    const uint8_t in[] = {0x01, END, 0x02};
    Encoder::PushPacketResult result = enc.pushPacket_ex(in, sizeof(in));
    
    EXPECT_TRUE(result.is_retry());
    EXPECT_EQ(result.status, WriteStatus::RetryLater);
    EXPECT_GT(result.consumed, 0u);
    EXPECT_LT(result.consumed, sizeof(in));
}

TEST(SLIPEncoderEnhanced, PushPacketExErrorFromOutput) {
    ErrorSink sink;
    sink.errorAfter = 3; // Error after 3 bytes
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 16, 4);

    const uint8_t in[] = {0x01, 0x02, 0x03, 0x04};
    Encoder::PushPacketResult result = enc.pushPacket_ex(in, sizeof(in));
    
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.status, WriteStatus::Error);
    EXPECT_EQ(result.error.code, ErrorCode::EncodeInternalError);
    EXPECT_STREQ(result.error.message, "Output function returned error");
}

TEST(SLIPEncoderEnhanced, PushPacketExPartialThenComplete) {
    BackpressuredSink sink;
    sink.acceptThenBlock = 2; // Accept 2 bytes then block
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 16, 4);

    const uint8_t in[] = {0x01, END, 0x02};
    
    // First push - should retry
    Encoder::PushPacketResult result1 = enc.pushPacket_ex(in, sizeof(in));
    EXPECT_TRUE(result1.is_retry());
    size_t consumed1 = result1.consumed;
    
    // Allow more bytes
    sink.acceptThenBlock = SIZE_MAX;
    
    // Second push - should complete
    Encoder::PushPacketResult result2 = enc.pushPacket_ex(in + consumed1, sizeof(in) - consumed1);
    EXPECT_TRUE(result2.is_success());
    EXPECT_EQ(consumed1 + result2.consumed, sizeof(in));
    
    // Verify final output has END
    ASSERT_FALSE(sink.out.empty());
    EXPECT_EQ(sink.out.back(), END);
}

TEST(SLIPEncoderEnhanced, PushPacketExMultiplePackets) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);

    const uint8_t in1[] = {0x01, 0x02};
    const uint8_t in2[] = {0x03, 0x04};
    
    Encoder::PushPacketResult result1 = enc.pushPacket_ex(in1, sizeof(in1));
    EXPECT_TRUE(result1.is_success());
    
    Encoder::PushPacketResult result2 = enc.pushPacket_ex(in2, sizeof(in2));
    EXPECT_TRUE(result2.is_success());
    
    // Should have two packets with END markers
    size_t expected_size = sizeof(in1) + 1 + sizeof(in2) + 1;
    EXPECT_EQ(sink.out.size(), expected_size);
}

// ============================================================================
// Queue management tests with enhanced reporting
// ============================================================================

TEST(SLIPEncoderEnhanced, QueueCapacityAndFree) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 8, 3);
    
    EXPECT_EQ(enc.capacity(), 8u);
    EXPECT_EQ(enc.queued(), 0u);
    EXPECT_EQ(enc.free(), 8u);
    
    // Push a simple packet that fits
    const uint8_t in[] = {0x01, 0x02};
    Encoder::PushPacketResult result = enc.pushPacket_ex(in, sizeof(in));
    
    // Should succeed and flush immediately
    EXPECT_TRUE(result.is_success() || result.is_retry());
    if (result.is_success()) {
        // After successful flush, queue should be empty
        EXPECT_EQ(enc.queued(), 0u);
    }
}

TEST(SLIPEncoderEnhanced, SetMaxSendChunk) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);
    
    enc.setMaxSendChunk(8);
    
    const uint8_t in[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    Encoder::PushPacketResult result = enc.pushPacket_ex(in, sizeof(in));
    
    EXPECT_TRUE(result.is_success());
    // The chunk size should be respected during flush
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(SLIPEncoderEnhanced, EmptyPacket) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 64, 16);
    
    Encoder::PushPacketResult result = enc.pushPacket_ex(nullptr, 0);
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.consumed, 0u);
    
    // Should output just END
    ASSERT_EQ(sink.out.size(), 1u);
    EXPECT_EQ(sink.out[0], END);
}

TEST(SLIPEncoderEnhanced, LargePacketWithEscapes) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 256, 64);
    
    std::vector<uint8_t> large_data(100, END); // All END bytes
    Encoder::PushPacketResult result = enc.pushPacket_ex(large_data.data(), large_data.size());
    
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.consumed, 100u);
    
    // Each END becomes ESC+ESCEND, plus final END
    // So 100 * 2 + 1 = 201 bytes
    EXPECT_EQ(sink.out.size(), 201u);
}

TEST(SLIPEncoderEnhanced, AllByteValues) {
    BackpressuredSink sink;
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 512, 64);
    
    std::vector<uint8_t> all_bytes(256);
    for (int i = 0; i < 256; i++) {
        all_bytes[i] = static_cast<uint8_t>(i);
    }
    
    Encoder::PushPacketResult result = enc.pushPacket_ex(all_bytes.data(), all_bytes.size());
    EXPECT_TRUE(result.is_success());
    EXPECT_EQ(result.consumed, 256u);
    
    // Verify output ends with END
    EXPECT_EQ(sink.out.back(), END);
}

// ============================================================================
// End pending state tests
// ============================================================================

TEST(SLIPEncoderEnhanced, EndPendingState) {
    BackpressuredSink sink;
    sink.acceptThenBlock = 100; // Block after some bytes
    Encoder enc([&sink](uint8_t b){ return sink(b); }, 8, 3);
    
    const uint8_t in[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    // First push - may leave END pending if buffer full
    Encoder::PushPacketResult result = enc.pushPacket_ex(in, sizeof(in));
    
    // Unblock and try again
    sink.acceptThenBlock = SIZE_MAX;
    Encoder::PushPacketResult result2 = enc.pushPacket_ex(nullptr, 0);
    
    // Should complete any pending END
    if (result.is_retry()) {
        EXPECT_TRUE(result2.is_success() || result2.is_retry());
    }
}
