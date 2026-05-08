/**
 * @file Error.hpp
 * @author Uli Köhler <github@techoverflow.net>
 * @version 1.0
 * @date 2025-08-19
 * 
 * Enhanced error reporting system for SLIPStream operations
 * 
 * @copyright Copyright (C) 2022..2025 Uli Köhler
 */
#pragma once
#include <cstdint>
#include <cstddef>

namespace SLIPStream {

/**
 * Detailed error codes for SLIP operations
 * Provides specific error information beyond simple success/failure
 */
enum class ErrorCode : uint8_t {
    Success = 0,
    
    // Encoding errors
    EncodeBufferTooSmall = 10,
    EncodeInternalError = 11,
    
    // Decoding errors
    DecodeBufferTooSmall = 20,
    DecodeNoEndMarker = 21,
    DecodeInvalidEscapeSequence = 22,
    DecodeTruncatedEscape = 23,
    DecodeInternalError = 24,
    
    // Buffer errors
    RXBufferOverflow = 30,
    
    // General errors
    UnknownError = 99
};

/**
 * Detailed error information structure
 * Provides context about what went wrong during an operation
 */
struct ErrorInfo {
    ErrorCode code;
    size_t position;  // Position in input buffer where error occurred (if applicable)
    const char* message;  // Human-readable error description
    
    ErrorInfo() : code(ErrorCode::Success), position(0), message("Success") {}
    ErrorInfo(ErrorCode c, size_t pos = 0, const char* msg = "")
        : code(c), position(pos), message(msg) {}
    
    bool is_success() const { return code == ErrorCode::Success; }
    bool is_error() const { return code != ErrorCode::Success; }
};

/**
 * Result type for operations that can fail
 * Contains either the result value or error information
 */
template<typename T>
struct Result {
    T value;
    ErrorInfo error;
    
    Result(T v) : value(v), error(ErrorCode::Success) {}
    Result(ErrorCode code, size_t pos = 0, const char* msg = "")
        : value(T()), error(code, pos, msg) {}
    
    bool is_success() const { return error.is_success(); }
    bool is_error() const { return error.is_error(); }
};

// Specialization for size_t (most common use case)
template<>
struct Result<size_t> {
    size_t value;
    ErrorInfo error;
    
    Result(size_t v) : value(v), error(ErrorCode::Success) {}
    Result(ErrorCode code, size_t pos = 0, const char* msg = "")
        : value(0), error(code, pos, msg) {}
    
    bool is_success() const { return error.is_success(); }
    bool is_error() const { return error.is_error(); }
};

/**
 * Convert ErrorCode to human-readable string
 */
const char* error_code_to_string(ErrorCode code);

/**
 * Get error message for a specific error code
 */
const char* get_error_message(ErrorCode code);

} // namespace SLIPStream
