#include "SLIPStream/SLIP.hpp"
#include "SLIPStream/SLIPBuffer.hpp"

size_t slip_encoded_length(const uint8_t* in, size_t inlen) {
	// This variable contains the length of the data
	size_t outlen = 0;
	const uint8_t* inend = in + inlen; // First character AFTER the
	for (; in < inend ; in++) {
		switch (*in) {
		case SLIP_END: // Need to escape END character to avoid RX seeing end of frame
			// Same as "case SLIP_ESC" so just continue.
		case SLIP_ESC: // Need to escape ESC character, we'll send
			outlen += 2; // Will send ESC + ESCESC
			break;
		default: // Any other character => will just copy
			outlen++; // Will only send the given character
		}
	}
	// + 1: SLIP END bytes
	return outlen + 1;
}

size_t slip_encode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen) {
	// This variable contains the length of the data
	uint8_t* out_start = out; // We will increment [out], hence copy the original value.

	// Output buffer must be AT LEAST as long as input data (sanity check)
	if(outlen < inlen) {
		return SLIP_ENCODE_ERROR;
	}

	uint8_t* outend = out + outlen; // First character AFTER the
	const uint8_t* inend = in + inlen; // First character AFTER the
	for (; in < inend ; in++) {
		switch (*in) {
		case SLIP_END: // Need to escape END character to avoid RX seeing end of frame
			// Check out of bounds memory acces
			if(out + 2 >= outend) {
				return SLIP_ENCODE_ERROR;
			}
			// Copy escaped END character
			*out++ = SLIP_ESC;
			*out++ = SLIP_ESCEND;
			break;
		case SLIP_ESC: // Need to escape ESC character, we'll send
			// Check out of bounds memory access
			if(out + 2 >= outend) {
				return SLIP_ENCODE_ERROR;
			}
			// Copy escaped END character
			*out++ = SLIP_ESC;
			*out++ = SLIP_ESCESC;
			break;
		default: // Any other character => just copy the character
			if(out + 1 >= outend) {
				return SLIP_ENCODE_ERROR;
			}
			*out++ = *in;
		}
	}
	// Check out of bounds access for END byte
	if(out + 1 > outend) { // NOTE: > instead of >= since there is only ONE character to be written
		return SLIP_ENCODE_ERROR;
	}
	// Insert END byte
	*out++ = SLIP_END;
	// Return number of bytes
	return (out - out_start);
}

size_t slip_decoded_length(const uint8_t* in, size_t inlen) {
	const uint8_t* p = in;
	const uint8_t* end = in + inlen;
	size_t outlen = 0;
	bool saw_end = false;
	while (p < end) {
		uint8_t c = *p++;
		if (c == SLIP_END) { // End of packet
			saw_end = true;
			break;
		}
		if (c == SLIP_ESC) {
			if (p >= end) {
				// ESC must be followed by a byte
				return SLIP_DECODE_ERROR;
			}
			uint8_t n = *p++;
			if (n == SLIP_ESCEND || n == SLIP_ESCESC) {
				outlen += 1;
			} else {
				// Invalid escape sequence
				return SLIP_DECODE_ERROR;
			}
		} else {
			outlen += 1;
		}
	}
	if (!saw_end) {
		// No END found
		return SLIP_DECODE_ERROR;
	}
	return outlen;
}

size_t slip_decode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen) {
	const uint8_t* p = in;
	const uint8_t* end = in + inlen;
	uint8_t* w = out;
	uint8_t* wend = out + outlen;
	bool saw_end = false;

	while (p < end) {
		uint8_t c = *p++;
		if (c == SLIP_END) {
			saw_end = true;
			break; // End of packet
		}
		if (c == SLIP_ESC) {
			if (p >= end) {
				return SLIP_DECODE_ERROR; // Truncated escape
			}
			uint8_t n = *p++;
			uint8_t decoded;
			if (n == SLIP_ESCEND) {
				decoded = SLIP_END;
			} else if (n == SLIP_ESCESC) {
				decoded = SLIP_ESC;
			} else {
				return SLIP_DECODE_ERROR; // Invalid escape sequence
			}
			if (w >= wend) {
				return SLIP_DECODE_ERROR; // Output buffer too small
			}
			*w++ = decoded;
		} else {
			if (w >= wend) {
				return SLIP_DECODE_ERROR; // Output buffer too small
			}
			*w++ = c;
		}
	}
	if (!saw_end) {
		return SLIP_DECODE_ERROR; // No END terminator found
	}
	return static_cast<size_t>(w - out);
}
