#include "SLIPStream/Error.hpp"

namespace SLIPStream {

const char* error_code_to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:
            return "Success";
        case ErrorCode::EncodeBufferTooSmall:
            return "EncodeBufferTooSmall";
        case ErrorCode::EncodeInternalError:
            return "EncodeInternalError";
        case ErrorCode::DecodeBufferTooSmall:
            return "DecodeBufferTooSmall";
        case ErrorCode::DecodeNoEndMarker:
            return "DecodeNoEndMarker";
        case ErrorCode::DecodeInvalidEscapeSequence:
            return "DecodeInvalidEscapeSequence";
        case ErrorCode::DecodeTruncatedEscape:
            return "DecodeTruncatedEscape";
        case ErrorCode::DecodeInternalError:
            return "DecodeInternalError";
        case ErrorCode::RXBufferOverflow:
            return "RXBufferOverflow";
        case ErrorCode::UnknownError:
            return "UnknownError";
        default:
            return "InvalidErrorCode";
    }
}

const char* get_error_message(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:
            return "Operation completed successfully";
        case ErrorCode::EncodeBufferTooSmall:
            return "Output buffer is too small to hold encoded data";
        case ErrorCode::EncodeInternalError:
            return "Internal encoding error occurred";
        case ErrorCode::DecodeBufferTooSmall:
            return "Output buffer is too small to hold decoded data";
        case ErrorCode::DecodeNoEndMarker:
            return "No END marker (0xC0) found in input data";
        case ErrorCode::DecodeInvalidEscapeSequence:
            return "Invalid escape sequence: ESC not followed by ESCEND or ESCESC";
        case ErrorCode::DecodeTruncatedEscape:
            return "Truncated escape sequence: ESC at end of input";
        case ErrorCode::DecodeInternalError:
            return "Internal decoding error occurred";
        case ErrorCode::RXBufferOverflow:
            return "RX buffer overflow during decoding";
        case ErrorCode::UnknownError:
            return "Unknown error occurred";
        default:
            return "Invalid error code";
    }
}

} // namespace SLIPStream
