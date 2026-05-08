#include "SLIPStream/SLIP.hpp"
#include "SLIPStream/Buffer.hpp"
#include "SLIPStream/Error.hpp"

namespace SLIPStream {

size_t encoded_length(const uint8_t* in, size_t inlen) {
	// This variable contains the length of the data
	size_t outlen = 0;
	const uint8_t* inend = in + inlen; // First character AFTER the
	for (; in < inend ; in++) {
		switch (*in) {
		case END: // Need to escape END character to avoid RX seeing end of frame
			// Same as "case SLIP_ESC" so just continue.
		case ESC: // Need to escape ESC character, we'll send
			outlen += 2; // Will send ESC + ESCESC
			break;
		default: // Any other character => will just copy
			outlen++; // Will only send the given character
		}
	}
	// + 1: SLIP END bytes
	return outlen + 1;
}

Result<size_t> encoded_length_ex(const uint8_t* in, size_t inlen) {
	// This variable contains the length of the data
	size_t outlen = 0;
	const uint8_t* inend = in + inlen; // First character AFTER the
	for (; in < inend ; in++) {
		switch (*in) {
		case END: // Need to escape END character to avoid RX seeing end of frame
			// Same as "case SLIP_ESC" so just continue.
		case ESC: // Need to escape ESC character, we'll send
			outlen += 2; // Will send ESC + ESCESC
			break;
		default: // Any other character => will just copy
			outlen++; // Will only send the given character
		}
	}
	// + 1: SLIP END bytes
	return Result<size_t>(outlen + 1);
}

size_t encode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen) {
	// This variable contains the length of the data
	uint8_t* out_start = out; // We will increment [out], hence copy the original value.

	// Output buffer must be AT LEAST as long as input data (sanity check)
	if(outlen < inlen) {
		return ENCODE_ERROR;
	}

	uint8_t* outend = out + outlen; // First character AFTER the
	const uint8_t* inend = in + inlen; // First character AFTER the
	for (; in < inend ; in++) {
		switch (*in) {
		case END: // Need to escape END character to avoid RX seeing end of frame
			// Check out of bounds memory acces
			if(out + 2 >= outend) {
				return ENCODE_ERROR;
			}
			// Copy escaped END character
			*out++ = ESC;
			*out++ = ESCEND;
			break;
		case ESC: // Need to escape ESC character, we'll send
			// Check out of bounds memory access
			if(out + 2 >= outend) {
				return ENCODE_ERROR;
			}
			// Copy escaped END character
			*out++ = ESC;
			*out++ = ESCESC;
			break;
		default: // Any other character => just copy the character
			if(out + 1 >= outend) {
				return ENCODE_ERROR;
			}
			*out++ = *in;
		}
	}
	// Check out of bounds access for END byte
	if(out + 1 > outend) { // NOTE: > instead of >= since there is only ONE character to be written
		return ENCODE_ERROR;
	}
	// Insert END byte
	*out++ = END;
	// Return number of bytes
	return (out - out_start);
}

Result<size_t> encode_packet_ex(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen) {
	// This variable contains the length of the data
	uint8_t* out_start = out; // We will increment [out], hence copy the original value.

	// Output buffer must be AT LEAST as long as input data (sanity check)
	if(outlen < inlen) {
		return Result<size_t>(ErrorCode::EncodeBufferTooSmall, 0, "Output buffer too small");
	}

	uint8_t* outend = out + outlen; // First character AFTER the
	const uint8_t* inend = in + inlen; // First character AFTER the
	size_t pos = 0;
	for (; in < inend ; in++) {
		switch (*in) {
		case END: // Need to escape END character to avoid RX seeing end of frame
			// Check out of bounds memory acces
			if(out + 2 >= outend) {
				return Result<size_t>(ErrorCode::EncodeBufferTooSmall, pos, "Output buffer too small for escaped END");
			}
			// Copy escaped END character
			*out++ = ESC;
			*out++ = ESCEND;
			break;
		case ESC: // Need to escape ESC character, we'll send
			// Check out of bounds memory access
			if(out + 2 >= outend) {
				return Result<size_t>(ErrorCode::EncodeBufferTooSmall, pos, "Output buffer too small for escaped ESC");
			}
			// Copy escaped END character
			*out++ = ESC;
			*out++ = ESCESC;
			break;
		default: // Any other character => just copy the character
			if(out + 1 >= outend) {
				return Result<size_t>(ErrorCode::EncodeBufferTooSmall, pos, "Output buffer too small for data byte");
			}
			*out++ = *in;
		}
		pos++;
	}
	// Check out of bounds access for END byte
	if(out + 1 > outend) { // NOTE: > instead of >= since there is only ONE character to be written
		return Result<size_t>(ErrorCode::EncodeBufferTooSmall, pos, "Output buffer too small for END byte");
	}
	// Insert END byte
	*out++ = END;
	// Return number of bytes
	return Result<size_t>(out - out_start);
}

size_t decoded_length(const uint8_t* in, size_t inlen) {
	const uint8_t* p = in;
	const uint8_t* end = in + inlen;
	size_t outlen = 0;
	bool saw_end = false;
	while (p < end) {
		uint8_t c = *p++;
		if (c == END) { // End of packet
			saw_end = true;
			break;
		}
		if (c == ESC) {
			if (p >= end) {
				// ESC must be followed by a byte
				return DECODE_ERROR;
			}
			uint8_t n = *p++;
			if (n == ESCEND || n == ESCESC) {
				outlen += 1;
			} else {
				// Invalid escape sequence
				return DECODE_ERROR;
			}
		} else {
			outlen += 1;
		}
	}
	if (!saw_end) {
		// No END found
		return DECODE_ERROR;
	}
	return outlen;
}

Result<size_t> decoded_length_ex(const uint8_t* in, size_t inlen) {
	const uint8_t* p = in;
	const uint8_t* end = in + inlen;
	size_t outlen = 0;
	bool saw_end = false;
	size_t pos = 0;
	while (p < end) {
		uint8_t c = *p++;
		if (c == END) { // End of packet
			saw_end = true;
			break;
		}
		if (c == ESC) {
			if (p >= end) {
				// ESC must be followed by a byte
				return Result<size_t>(ErrorCode::DecodeTruncatedEscape, pos, "Truncated escape sequence at end of input");
			}
			uint8_t n = *p++;
			if (n == ESCEND || n == ESCESC) {
				outlen += 1;
			} else {
				// Invalid escape sequence
				return Result<size_t>(ErrorCode::DecodeInvalidEscapeSequence, pos, "Invalid escape sequence: ESC not followed by ESCEND or ESCESC");
			}
		} else {
			outlen += 1;
		}
		pos++;
	}
	if (!saw_end) {
		// No END found
		return Result<size_t>(ErrorCode::DecodeNoEndMarker, pos, "No END marker (0xC0) found in input data");
	}
	return Result<size_t>(outlen);
}

size_t decode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen) {
	const uint8_t* p = in;
	const uint8_t* end = in + inlen;
	uint8_t* w = out;
	uint8_t* wend = out + outlen;
	bool saw_end = false;

	while (p < end) {
		uint8_t c = *p++;
		if (c == END) {
			saw_end = true;
			break; // End of packet
		}
		if (c == ESC) {
			if (p >= end) {
				return DECODE_ERROR; // Truncated escape
			}
			uint8_t n = *p++;
			uint8_t decoded;
			if (n == ESCEND) {
				decoded = END;
			} else if (n == ESCESC) {
				decoded = ESC;
			} else {
				return DECODE_ERROR; // Invalid escape sequence
			}
			if (w >= wend) {
				return DECODE_ERROR; // Output buffer too small
			}
			*w++ = decoded;
		} else {
			if (w >= wend) {
				return DECODE_ERROR; // Output buffer too small
			}
			*w++ = c;
		}
	}
	if (!saw_end) {
		return DECODE_ERROR; // No END terminator found
	}
	return static_cast<size_t>(w - out);
}

Result<size_t> decode_packet_ex(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen) {
	const uint8_t* p = in;
	const uint8_t* end = in + inlen;
	uint8_t* w = out;
	uint8_t* wend = out + outlen;
	bool saw_end = false;
	size_t pos = 0;

	while (p < end) {
		uint8_t c = *p++;
		if (c == END) {
			saw_end = true;
			break; // End of packet
		}
		if (c == ESC) {
			if (p >= end) {
				return Result<size_t>(ErrorCode::DecodeTruncatedEscape, pos, "Truncated escape sequence at end of input");
			}
			uint8_t n = *p++;
			uint8_t decoded;
			if (n == ESCEND) {
				decoded = END;
			} else if (n == ESCESC) {
				decoded = ESC;
			} else {
				return Result<size_t>(ErrorCode::DecodeInvalidEscapeSequence, pos, "Invalid escape sequence: ESC not followed by ESCEND or ESCESC");
			}
			if (w >= wend) {
				return Result<size_t>(ErrorCode::DecodeBufferTooSmall, pos, "Output buffer too small for decoded data");
			}
			*w++ = decoded;
		} else {
			if (w >= wend) {
				return Result<size_t>(ErrorCode::DecodeBufferTooSmall, pos, "Output buffer too small for data byte");
			}
			*w++ = c;
		}
		pos++;
	}
	if (!saw_end) {
		return Result<size_t>(ErrorCode::DecodeNoEndMarker, pos, "No END terminator found in input data");
	}
	return Result<size_t>(static_cast<size_t>(w - out));
}

} // namespace SLIPStream
