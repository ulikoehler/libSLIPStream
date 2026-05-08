/**
 * @file CRC32.hpp
 * @author Uli Köhler <github@techoverflow.net>
 * @version 1.0
 * @date 2025-08-19
 *
 * CRC32 calculation using Ethernet polynomial
 *
 * @copyright Copyright (C) 2022..2025 Uli Köhler
 */
#pragma once
#include <cstdint>
#include <cstddef>

namespace SLIPStream {

/**
 * Calculate CRC32 checksum using Ethernet polynomial (0x04C11DB7)
 * 
 * Specification:
 * - Polynomial: 0x04C11DB7 (Ethernet polynomial)
 * - Initial value: 0xFFFFFFFF
 * - Final XOR: 0xFFFFFFFF
 * - Bit order: MSB-first (big-endian per polynomial definition)
 * - Input reflection: None
 * - Output reflection: None
 * 
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return CRC32 checksum
 */
uint32_t calculate_crc32(const uint8_t* data, size_t length);

/**
 * Calculate CRC32 checksum with initial value
 * 
 * Allows for incremental CRC calculation by passing previous result as initial value.
 * 
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @param initial_crc Initial CRC value (use 0xFFFFFFFF for standard calculation)
 * @return CRC32 checksum
 */
uint32_t calculate_crc32_with_initial(const uint8_t* data, size_t length, uint32_t initial_crc);

/**
 * Append CRC32 checksum to data in little-endian format
 * 
 * This function calculates the CRC32 of the data and appends it to the end
 * of the buffer in little-endian byte order (LSB first).
 * 
 * @param data Pointer to data buffer (must have at least length+4 bytes available)
 * @param length Length of data in bytes (will be increased by 4)
 * @return New total length (length + 4)
 */
size_t append_crc32(uint8_t* data, size_t length);

/**
 * Extract CRC32 checksum from data in little-endian format
 * 
 * Extracts the last 4 bytes of the data as a little-endian CRC32 value.
 * 
 * @param data Pointer to data buffer
 * @param length Length of data in bytes (must be at least 4)
 * @param crc_out Pointer to store extracted CRC32 value
 * @return Length of data without CRC (length - 4), or 0 on error
 */
size_t extract_crc32(const uint8_t* data, size_t length, uint32_t* crc_out);

/**
 * Verify CRC32 checksum in data
 * 
 * Calculates CRC32 of the data (excluding last 4 bytes) and compares it
 * with the CRC32 stored in the last 4 bytes (little-endian format).
 * 
 * @param data Pointer to data buffer
 * @param length Length of data in bytes (must be at least 4)
 * @return true if CRC32 matches, false otherwise
 */
bool verify_crc32(const uint8_t* data, size_t length);

} // namespace SLIPStream
