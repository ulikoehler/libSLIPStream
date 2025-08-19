#include "SLIPStream/SLIP.hpp"
#include "SLIPStream/SLIPBuffer.hpp"

const char ESCEND_BUF[] = {SLIP_ESC, SLIP_ESCEND};
const char ESCESC_BUF[] = {SLIP_ESC, SLIP_ESCESC};

const char ESCEND_BUF[] = {SLIP_ESC, SLIP_ESCEND};
const char ESCESC_BUF[] = {SLIP_ESC, SLIP_ESCESC};

size_t slip_packet_length(const uint8_t* in, size_t inlen) {
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
