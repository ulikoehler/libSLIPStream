/**
 * @file SLIPStream.hpp
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

// Return value of slip_encode_packet if the output buffer is not large enough
#define SLIP_ENCODE_ERROR SIZE_MAX // Maximum value of size_t

enum class SLIPLogType: uint8_t {
    Unknown = 0,
    RXBufferOverflow = 1
};

/**
 * A stateful decoder for SLIP messages that receives
 */
class SLIPStreamDecoder {
public:
    /**
     * @brief Construct a new SLIPStreamDecoder object
     * 
     * @param rxbuf Receive buffer with appropriate size to store decoded messages. Must hold the largest available message
     * @param rxbufSize The available size of rxbuf
     * @param messageCallback This function will be called once a full message has been received
     */
    SLIPStreamDecoder(uint8_t* rxbuf, size_t rxbufSize, std::function<void(uint8_t*, size_t)> messageCallback, std::function<void(SLIPLogType, const char*)> logCallback);
    
    /**
     * @brief Process multiple received bytes
     * 
     * @param data 
     * @param size 
     */
    void consume(const uint8_t* data, size_t size);
    void consume(uint8_t byte);

    /**
     * Clear RX buf etc
     */
    void reset();
    
private:
    bool lastCharIsEsc;
    uint8_t* rxbuf;
    size_t rxbufPos;
    size_t rxbufSize;
    std::function<void(uint8_t*, size_t)> messageCallback;
    std::function<void(SLIPLogType, const char*)> logCallback;

    // The following are for logging only
    const char* logTag; // Tag for logging, like "ZMCU-SLIP"
};
