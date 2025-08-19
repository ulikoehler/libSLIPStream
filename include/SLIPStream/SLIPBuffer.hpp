/**
 * @file SLIPBuffer.hpp
 * @author Uli Köhler <github@techoverflow.net>
 * @version 1.0
 * @date 2025-08-19
 *
 * Buffer-based methods for working with SLIP data
 *
 * @copyright Copyright (C) 2022..2025 Uli Köhler
 */
#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include "SLIPStream/SLIP.hpp"


// Return value of slip_encode_packet if the output buffer is not large enough
#define SLIP_ENCODE_ERROR SIZE_MAX // Maximum value of size_t

// Return value of slip_decode_packet/slip_decoded_length for errors
#define SLIP_DECODE_ERROR SIZE_MAX // Maximum value of size_t

enum class SLIPLogType: uint8_t {
	Unknown = 0,
	RXBufferOverflow = 1
};

/**
 * Determine the packet length when sending the given buffer [p] using SLIP.
 * This DOES NOT ENCODE ANYTHING, it just determines the length
 * which an encoded version of the data would have
 * @return The number of bytes a SLIP encoded version of [in] would consume
 */
size_t slip_encoded_length(const uint8_t* in, size_t inlen);

/**
 * Encode given input data using SLIP, saving it into the given output buffer.
 * WARNING! The output buffer MUST have a length of at least the return value of
 *    slip_packet_len(in, inlen)
 * In case the output buffer is not large enough, this function will return SLIP_ENCODE_ERROR and
 * the output buffer will be left in an undefined space.
 * Take special care that the input data does not change between the calls to
 * slip_packet_len() and slip_encode_packet()
 * @return The number of bytes in [out], or SLIP_ENCODE_ERROR
 */
size_t slip_encode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen);

/**
 * Determine the decoded packet length when receiving a SLIP-encoded buffer [in].
 * Decoding will stop at the first SLIP END byte (0xC0). Escapes are resolved
 * (ESC, ESCEND) -> END and (ESC, ESCESC) -> ESC.
 *
 * Error conditions:
 *  - Returns SLIP_DECODE_ERROR if no END byte is present in [in]
 *  - Returns SLIP_DECODE_ERROR for malformed escape sequences (ESC not followed by ESCEND/ESCESC)
 *
 * This function does not modify the input.
 * @return The number of decoded bytes up to the first END, or SLIP_DECODE_ERROR on error.
 */
size_t slip_decoded_length(const uint8_t* in, size_t inlen);

/**
 * Decode a SLIP-encoded buffer [in] into the provided output buffer [out].
 * Decoding stops at the first END byte in [in].
 *
 * The caller should first call slip_decoded_length(in, inlen) to determine the
 * required size of [out]; then allocate a buffer of at least that size and call
 * this function.
 *
 * Error conditions:
 *  - Returns SLIP_DECODE_ERROR if output buffer [outlen] is too small
 *  - Returns SLIP_DECODE_ERROR if no END byte is present in [in]
 *  - Returns SLIP_DECODE_ERROR for malformed escape sequences
 *
 * @return The number of bytes written to [out], or SLIP_DECODE_ERROR on error.
 */
size_t slip_decode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen);
