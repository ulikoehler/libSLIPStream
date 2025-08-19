/**
 * @file SLIP.hpp
 * @author Uli Köhler <github@techoverflow.net>
 * @version 1.0
 * @date 2025-08-19
 * 
 * Common definitions and functions for working with SLIP data
 * 
 * @copyright Copyright (C) 2022..2025 Uli Köhler
 */
#pragma once
#include <cstdint>

// SLIP special character definitions
inline constexpr uint8_t SLIP_END    = 0xC0; // SLIP end of packet character
inline constexpr uint8_t SLIP_ESC    = 0xDB; // SLIP escape character
inline constexpr uint8_t SLIP_ESCEND = 0xDC; // Escaped END data byte
inline constexpr uint8_t SLIP_ESCESC = 0xDD; // Escaped ESC data byte
