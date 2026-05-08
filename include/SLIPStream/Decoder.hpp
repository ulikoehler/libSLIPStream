/**
 * @file SLIPStreamDecoder.hpp
 * @author Uli Köhler <github@techoverflow.net>
 * @version 1.0
 * @date 2025-08-19
 * 
 * @copyright Copyright (C) 2022..2025 Uli Köhler
 */
#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>
#include "SLIPStream/SLIP.hpp"
#include "SLIPStream/Error.hpp"

namespace SLIPStream {

enum class LogType: uint8_t {
    Unknown = 0,
    RXBufferOverflow = 1
};

/**
 * Enhanced log information with error details
 */
struct LogInfo {
    LogType type;
    ErrorInfo error;
    const char* message;
    
    LogInfo(LogType t, const char* msg = "") : type(t), error(ErrorCode::Success), message(msg) {}
    LogInfo(ErrorCode code, size_t pos = 0, const char* msg = "")
        : type(LogType::Unknown), error(code, pos, msg), message(msg) {}
};

// Stateful decoder for SLIP messages
class Decoder {
public:
    Decoder(uint8_t* rxbuf, size_t rxbufSize, std::function<void(uint8_t*, size_t)> messageCallback, std::function<void(LogType, const char*)> logCallback);
    
    // Enhanced constructor with error callback
    Decoder(uint8_t* rxbuf, size_t rxbufSize, std::function<void(uint8_t*, size_t)> messageCallback, std::function<void(LogInfo)> logCallbackEx);

    void consume(const uint8_t* data, size_t size);
    void consume(uint8_t byte);
    
    // Enhanced consume with error reporting
    struct ConsumeResult {
        size_t consumed;
        ErrorInfo error;
        bool has_error;
        
        ConsumeResult(size_t c) : consumed(c), error(ErrorCode::Success), has_error(false) {}
        ConsumeResult(ErrorCode code, size_t c, size_t pos = 0, const char* msg = "")
            : consumed(c), error(code, pos, msg), has_error(true) {}
    };
    ConsumeResult consume_ex(const uint8_t* data, size_t size);
    ConsumeResult consume_ex(uint8_t byte);

    // Clear RX buf etc
    void reset();
    
    // Get last error information
    ErrorInfo getLastError() const { return lastError; }
    
private:
    bool lastCharIsEsc;
    uint8_t* rxbuf;
    size_t rxbufPos;
    size_t rxbufSize;
    std::function<void(uint8_t*, size_t)> messageCallback;
    std::function<void(LogType, const char*)> logCallback;
    std::function<void(LogInfo)> logCallbackEx;
    
    // Error tracking
    ErrorInfo lastError;
    size_t consumedCount; // Track bytes consumed for error position

    // The following are for logging only
    const char* logTag; // Tag for logging, like "ZMCU-SLIP"
};

} // namespace SLIPStream
