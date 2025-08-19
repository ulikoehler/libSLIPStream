/**
 * @file SLIPStreamEncoder.hpp
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

/**
 * Non-blocking write status for the encoder's single-byte output callback
 */
enum class SLIPWriteStatus: uint8_t {
    Ok = 0,       // Byte accepted and written
    RetryLater = 1, // Output would block; try again later without data loss
    Error = 2     // Non-recoverable error from output
};

using SLIPOutputFn = std::function<SLIPWriteStatus(uint8_t)>;

/**
 * A stateful, non-blocking SLIP encoder with internal buffering.
 *
 * Contract:
 * - Use pushPacket() to encode and send a payload as a SLIP packet (END terminator appended).
 * - The provided output callback may return RetryLater; in that case push/flush stop early
 *   and you can resume by calling pushPacket()/flush() again.
 * - Internally queued (already-encoded) bytes are preserved until sent.
 */
class SLIPStreamEncoder {
public:
    /**
     * @brief Construct a new SLIPStreamEncoder
     * 
     * @param outputFn Single-byte output function. Returns SLIPWriteStatus to support RetryLater.
     * @param txBufferSize Size of internal transmit buffer (encoded bytes queue).
     * @param maxSendChunk How many bytes to try to send per flush()/push() call for fairness.
     */
    SLIPStreamEncoder(SLIPOutputFn outputFn, size_t txBufferSize, size_t maxSendChunk = 64);

    /**
     * @brief Attempt to flush up to maxSendChunk queued encoded bytes via outputFn.
     * @return SLIPWriteStatus Ok if all attempted bytes were written or queue empty,
     *         RetryLater if output back-pressured before completing,
     *         Error on output error.
     */
    SLIPWriteStatus flush();

    /**
     * @brief Encode and queue a complete SLIP packet (payload escaped, END appended).
     *        This function is cooperative/non-blocking and may return early.
     * @param data Pointer to payload bytes.
     * @param size Number of payload bytes.
     * @return pair of (status, consumedBytes). 'consumedBytes' is how many input bytes
     *         were accepted/encoded this call. If status==RetryLater, call again with the
     *         remaining bytes starting at data+consumedBytes. END will be appended once
     *         all bytes have been consumed; that END may also require additional calls to send.
     */
    std::pair<SLIPWriteStatus, size_t> pushPacket(const uint8_t* data, size_t size);

    /**
     * @brief Configure how many bytes to try to send at once when flushing.
     */
    void setMaxSendChunk(size_t n) { maxSendChunk = n; }

    size_t queued() const { return txSize; }
    size_t capacity() const { return txBuf.size(); }
    size_t free() const { return txBuf.size() - txSize; }

private:
    // Internal helpers for queue management
    bool queueByte(uint8_t b);
    bool dequeueByte(uint8_t& b);

    // Try to ensure at least 'n' bytes free in queue by flushing as needed
    // Returns Ok/RetryLater/Error per output.
    SLIPWriteStatus ensureFree(size_t n);

    // Encode one payload byte into queue (may be 1 or 2 bytes)
    // Returns Ok/RetryLater/Error. Sets encodedCount to how many payload bytes were consumed (0 or 1).
    SLIPWriteStatus encodeOne(const uint8_t* data, size_t size, size_t& consumed);

    SLIPOutputFn outputFn;
    std::vector<uint8_t> txBuf;
    size_t txHead; // pop index
    size_t txTail; // push index
    size_t txSize; // number of bytes currently queued
    size_t maxSendChunk;

    // Packet state: whether we still need to append a trailing END for the current packet
    bool endPending;
};