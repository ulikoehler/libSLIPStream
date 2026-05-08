// Comprehensive tests for the Error reporting system
#include <gtest/gtest.h>
#include "SLIPStream/Error.hpp"

using namespace SLIPStream;

TEST(ErrorSystem, ErrorCodeToString) {
    EXPECT_STREQ(error_code_to_string(ErrorCode::Success), "Success");
    EXPECT_STREQ(error_code_to_string(ErrorCode::EncodeBufferTooSmall), "EncodeBufferTooSmall");
    EXPECT_STREQ(error_code_to_string(ErrorCode::DecodeNoEndMarker), "DecodeNoEndMarker");
    EXPECT_STREQ(error_code_to_string(ErrorCode::DecodeInvalidEscapeSequence), "DecodeInvalidEscapeSequence");
    EXPECT_STREQ(error_code_to_string(ErrorCode::RXBufferOverflow), "RXBufferOverflow");
    EXPECT_STREQ(error_code_to_string(ErrorCode::UnknownError), "UnknownError");
}

TEST(ErrorSystem, GetErrorMessage) {
    EXPECT_STREQ(get_error_message(ErrorCode::Success), "Operation completed successfully");
    EXPECT_STREQ(get_error_message(ErrorCode::EncodeBufferTooSmall), "Output buffer is too small to hold encoded data");
    EXPECT_STREQ(get_error_message(ErrorCode::DecodeNoEndMarker), "No END marker (0xC0) found in input data");
    EXPECT_STREQ(get_error_message(ErrorCode::DecodeInvalidEscapeSequence), "Invalid escape sequence: ESC not followed by ESCEND or ESCESC");
    EXPECT_STREQ(get_error_message(ErrorCode::RXBufferOverflow), "RX buffer overflow during decoding");
}

// ErrorInfoConstruction test removed - LogInfo is tested in test_decoder_enhanced.cpp

TEST(ErrorSystem, ResultConstruction) {
    Result<size_t> result1(100);
    EXPECT_EQ(result1.value, 100u);
    EXPECT_TRUE(result1.is_success());
    EXPECT_FALSE(result1.is_error());
    EXPECT_EQ(result1.error.code, ErrorCode::Success);

    Result<size_t> result2(ErrorCode::DecodeBufferTooSmall, 10, "Buffer too small");
    EXPECT_EQ(result2.value, 0u);
    EXPECT_FALSE(result2.is_success());
    EXPECT_TRUE(result2.is_error());
    EXPECT_EQ(result2.error.code, ErrorCode::DecodeBufferTooSmall);
    EXPECT_EQ(result2.error.position, 10u);
    EXPECT_STREQ(result2.error.message, "Buffer too small");
}

TEST(ErrorSystem, ResultWithValue) {
    Result<size_t> result(256);
    EXPECT_EQ(result.value, 256u);
    EXPECT_TRUE(result.is_success());
}

TEST(ErrorSystem, ResultWithError) {
    Result<size_t> result(ErrorCode::EncodeInternalError, 5, "Internal error");
    EXPECT_EQ(result.value, 0u);
    EXPECT_FALSE(result.is_success());
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error.code, ErrorCode::EncodeInternalError);
    EXPECT_EQ(result.error.position, 5u);
    EXPECT_STREQ(result.error.message, "Internal error");
}

// LogInfo is tested in test_decoder_enhanced.cpp as it's part of the Decoder API
