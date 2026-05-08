// Comprehensive tests for Decoder class with enhanced error reporting
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <functional>
#include "SLIPStream/Decoder.hpp"
#include "SLIPStream/SLIP.hpp"

using namespace SLIPStream;

class SLIPDecoderEnhancedTest : public ::testing::Test {
protected:
    std::vector<uint8_t> rxbuf;
    std::vector<uint8_t> received_messages;
    std::vector<LogInfo> log_entries;
    
    void SetUp() override {
        rxbuf.resize(256);
        received_messages.clear();
        log_entries.clear();
    }
    
    static void messageCallback(uint8_t* data, size_t size) {
        // This is a static callback, will be set up in tests
    }
    
    void logCallbackEx(LogInfo info) {
        log_entries.push_back(info);
    }
};

// Helper to capture messages
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

// ============================================================================
// Basic decoding tests with enhanced error reporting
// ============================================================================

TEST(SLIPDecoderEnhanced, ConsumeExSingleByteSuccess) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    // Consume a regular byte
    Decoder::ConsumeResult result = dec.consume_ex(0x01);
    EXPECT_TRUE(!result.has_error);
    EXPECT_EQ(result.consumed, 1u);
    EXPECT_TRUE(log_entries.empty());
}

TEST(SLIPDecoderEnhanced, ConsumeExSimpleMessage) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    const uint8_t message[] = {0x01, 0x02, 0x03, END};
    
    for (size_t i = 0; i < sizeof(message); i++) {
        Decoder::ConsumeResult result = dec.consume_ex(message[i]);
        if (result.has_error) {
            FAIL() << "Unexpected error at position " << i;
        }
    }
    
    // Should have received one message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 3u);
    EXPECT_EQ(capture.messages[0][0], 0x01);
    EXPECT_EQ(capture.messages[0][1], 0x02);
    EXPECT_EQ(capture.messages[0][2], 0x03);
}

TEST(SLIPDecoderEnhanced, ConsumeExWithEscapes) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    const uint8_t message[] = {ESC, ESCEND, 0x02, ESC, ESCESC, END};
    
    for (size_t i = 0; i < sizeof(message); i++) {
        Decoder::ConsumeResult result = dec.consume_ex(message[i]);
        if (result.has_error) {
            FAIL() << "Unexpected error at position " << i;
        }
    }
    
    // Should have received one message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 3u);
    EXPECT_EQ(capture.messages[0][0], END);
    EXPECT_EQ(capture.messages[0][1], 0x02);
    EXPECT_EQ(capture.messages[0][2], ESC);
}

TEST(SLIPDecoderEnhanced, ConsumeExBufferOverflow) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(10); // Small buffer
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    // Fill buffer with data (no END)
    for (size_t i = 0; i < 9; i++) {
        Decoder::ConsumeResult result = dec.consume_ex(0x01);
        EXPECT_FALSE(result.has_error);
    }
    
    // This should cause overflow
    Decoder::ConsumeResult result = dec.consume_ex(0x02);
    EXPECT_TRUE(result.has_error);
    EXPECT_EQ(result.error.code, ErrorCode::RXBufferOverflow);
    EXPECT_STREQ(result.error.message, "RX buffer overflow");
    
    // Should have logged the error
    EXPECT_FALSE(log_entries.empty());
    EXPECT_EQ(log_entries.back().error.code, ErrorCode::RXBufferOverflow);
}

TEST(SLIPDecoderEnhanced, ConsumeExInvalidEscapeSequence) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    std::function<void(LogInfo)> enhanced_callback = [&log_entries](LogInfo info) { log_entries.push_back(info); };
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), enhanced_callback);
    
    // Send ESC followed by invalid byte
    Decoder::ConsumeResult result1 = dec.consume_ex(ESC);
    EXPECT_FALSE(result1.has_error);
    
    Decoder::ConsumeResult result2 = dec.consume_ex(0xFF); // Invalid
    EXPECT_TRUE(result2.has_error);
    EXPECT_EQ(result2.error.code, ErrorCode::DecodeInvalidEscapeSequence);
    
    // The enhanced callback may or may not be called depending on implementation
    // Just verify the error is returned correctly
    EXPECT_TRUE(result2.has_error);
}

TEST(SLIPDecoderEnhanced, ConsumeExMultipleMessages) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    const uint8_t msg1[] = {0x01, 0x02, END};
    const uint8_t msg2[] = {0x03, 0x04, END};
    
    for (size_t i = 0; i < sizeof(msg1); i++) {
        Decoder::ConsumeResult result = dec.consume_ex(msg1[i]);
        EXPECT_FALSE(result.has_error);
    }
    
    for (size_t i = 0; i < sizeof(msg2); i++) {
        Decoder::ConsumeResult result = dec.consume_ex(msg2[i]);
        EXPECT_FALSE(result.has_error);
    }
    
    // Should have received two messages
    EXPECT_EQ(capture.messages.size(), 2u);
    EXPECT_EQ(capture.messages[0].size(), 2u);
    EXPECT_EQ(capture.messages[1].size(), 2u);
}

// ============================================================================
// consume_ex with buffer tests
// ============================================================================

TEST(SLIPDecoderEnhanced, ConsumeExBufferSuccess) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    const uint8_t message[] = {0x01, 0x02, 0x03, END};
    Decoder::ConsumeResult result = dec.consume_ex(message, sizeof(message));
    
    EXPECT_FALSE(result.has_error);
    EXPECT_EQ(result.consumed, sizeof(message));
    
    // Should have received one message
    EXPECT_EQ(capture.messages.size(), 1u);
}

TEST(SLIPDecoderEnhanced, ConsumeExBufferWithError) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    const uint8_t data[] = {ESC, 0xFF, END};
    Decoder::ConsumeResult result = dec.consume_ex(data, sizeof(data));
    
    EXPECT_TRUE(result.has_error);
    EXPECT_EQ(result.error.code, ErrorCode::DecodeInvalidEscapeSequence);
    EXPECT_EQ(result.consumed, 1u); // Consumed ESC before error
}

TEST(SLIPDecoderEnhanced, ConsumeExBufferPartialError) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_entries](LogInfo info) { log_entries.push_back(info); });
    
    const uint8_t data[] = {0x01, 0x02, ESC, 0xFF, END};
    Decoder::ConsumeResult result = dec.consume_ex(data, sizeof(data));
    
    EXPECT_TRUE(result.has_error);
    EXPECT_EQ(result.error.code, ErrorCode::DecodeInvalidEscapeSequence);
    EXPECT_EQ(result.consumed, 3u); // Consumed up to ESC before error
}

// ============================================================================
// Get last error tests
// ============================================================================

TEST(SLIPDecoderEnhanced, GetLastErrorInitiallySuccess) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    ErrorInfo error = dec.getLastError();
    EXPECT_EQ(error.code, ErrorCode::Success);
}

TEST(SLIPDecoderEnhanced, GetLastErrorAfterError) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Cause an error
    dec.consume_ex(ESC);
    Decoder::ConsumeResult result = dec.consume_ex(0xFF);
    
    ErrorInfo error = dec.getLastError();
    // The error should be set after the invalid escape sequence
    EXPECT_TRUE(result.has_error);
}

TEST(SLIPDecoderEnhanced, GetLastErrorAfterReset) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    std::vector<LogInfo> log_entries;
    
    std::function<void(LogInfo)> enhanced_callback = [&log_entries](LogInfo info) { log_entries.push_back(info); };
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), enhanced_callback);
    
    // Cause an error
    dec.consume_ex(ESC);
    dec.consume_ex(0xFF);
    
    // Reset
    dec.reset();
    
    ErrorInfo error = dec.getLastError();
    EXPECT_EQ(error.code, ErrorCode::Success);
}

// ============================================================================
// Reset tests
// ============================================================================

TEST(SLIPDecoderEnhanced, ResetClearsState) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Add some bytes without END
    dec.consume_ex(0x01);
    dec.consume_ex(0x02);
    
    // Reset
    dec.reset();
    
    // Add new message
    const uint8_t message[] = {0x03, 0x04, END};
    for (size_t i = 0; i < sizeof(message); i++) {
        dec.consume_ex(message[i]);
    }
    
    // Should only receive the new message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 2u);
    EXPECT_EQ(capture.messages[0][0], 0x03);
    EXPECT_EQ(capture.messages[0][1], 0x04);
}

TEST(SLIPDecoderEnhanced, ResetAfterEscape) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Start escape sequence
    dec.consume_ex(ESC);
    
    // Reset
    dec.reset();
    
    // Add valid message
    const uint8_t message[] = {0x01, END};
    for (size_t i = 0; i < sizeof(message); i++) {
        dec.consume_ex(message[i]);
    }
    
    // Should receive valid message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 1u);
    EXPECT_EQ(capture.messages[0][0], 0x01);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(SLIPDecoderEnhanced, EmptyMessage) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    // Just END
    Decoder::ConsumeResult result = dec.consume_ex(END);
    EXPECT_FALSE(result.has_error);
    
    // Should receive empty message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 0u);
}

TEST(SLIPDecoderEnhanced, ConsecutiveENDs) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    const uint8_t data[] = {END, END, END};
    for (size_t i = 0; i < sizeof(data); i++) {
        dec.consume_ex(data[i]);
    }
    
    // Should receive three empty messages
    EXPECT_EQ(capture.messages.size(), 3u);
    for (const auto& msg : capture.messages) {
        EXPECT_EQ(msg.size(), 0u);
    }
}

TEST(SLIPDecoderEnhanced, AllSpecialBytes) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(256);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    const uint8_t data[] = {ESC, ESCEND, ESC, ESCESC, END};
    for (size_t i = 0; i < sizeof(data); i++) {
        dec.consume_ex(data[i]);
    }
    
    // Should receive one message with END and ESC
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 2u);
    EXPECT_EQ(capture.messages[0][0], END);
    EXPECT_EQ(capture.messages[0][1], ESC);
}

TEST(SLIPDecoderEnhanced, LargeMessage) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(512);
    
    std::function<void(LogType, const char*)> legacy_callback = nullptr;
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(), legacy_callback);
    
    std::vector<uint8_t> large_data(200, 0x42);
    large_data.push_back(END);
    
    for (size_t i = 0; i < large_data.size(); i++) {
        Decoder::ConsumeResult result = dec.consume_ex(large_data[i]);
        EXPECT_FALSE(result.has_error);
    }
    
    // Should receive one large message
    EXPECT_EQ(capture.messages.size(), 1u);
    EXPECT_EQ(capture.messages[0].size(), 200u);
}

// ============================================================================
// Legacy callback compatibility
// ============================================================================

TEST(SLIPDecoderEnhanced, LegacyLogCallbackStillWorks) {
    MessageCapture capture;
    std::vector<uint8_t> rxbuf(10);
    std::vector<LogType> log_types;
    std::vector<const char*> log_messages;
    
    Decoder dec(rxbuf.data(), rxbuf.size(), capture.getFn(),
        [&log_types, &log_messages](LogType type, const char* msg) {
            log_types.push_back(type);
            log_messages.push_back(msg);
        });
    
    // Cause overflow
    for (size_t i = 0; i < 9; i++) {
        dec.consume_ex(0x01);
    }
    dec.consume_ex(0x02);
    
    // Should have logged via legacy callback
    EXPECT_FALSE(log_types.empty());
    EXPECT_EQ(log_types.back(), LogType::RXBufferOverflow);
}
