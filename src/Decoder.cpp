#include "SLIPStream/SLIP.hpp"
#include "SLIPStream/Decoder.hpp"
#include "SLIPStream/Error.hpp"

namespace SLIPStream {

Decoder::Decoder(uint8_t* rxbuf, size_t rxbufSize, std::function<void(uint8_t*, size_t)> messageCallback, std::function<void(LogType, const char*)> logCallback)
    : lastCharIsEsc(false), rxbuf(rxbuf), rxbufPos(0), rxbufSize(rxbufSize), messageCallback(messageCallback), logCallback(logCallback), logCallbackEx(nullptr), lastError(ErrorCode::Success), consumedCount(0) {
}

Decoder::Decoder(uint8_t* rxbuf, size_t rxbufSize, std::function<void(uint8_t*, size_t)> messageCallback, std::function<void(LogInfo)> logCallbackEx)
    : lastCharIsEsc(false), rxbuf(rxbuf), rxbufPos(0), rxbufSize(rxbufSize), messageCallback(messageCallback), logCallback(nullptr), logCallbackEx(logCallbackEx), lastError(ErrorCode::Success), consumedCount(0) {
}


void Decoder::consume(const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++)
    {
        consume(data[i]);
    }
    
}

size_t Decoder::consume_chunk(const uint8_t* data, size_t size, size_t chunk_size) {
    size_t consumed = 0;
    while (consumed < size) {
        size_t to_consume = std::min(chunk_size, size - consumed);
        consume(data + consumed, to_consume);
        consumed += to_consume;
    }
    return consumed;
}

Decoder::ConsumeResult Decoder::consume_ex(const uint8_t* data, size_t size) {
    size_t consumed = 0;
    for (size_t i = 0; i < size; i++)
    {
        ConsumeResult result = consume_ex(data[i]);
        if (result.has_error) {
            return ConsumeResult(result.error.code, consumed, result.error.position, result.error.message);
        }
        consumed++;
    }
    return ConsumeResult(consumed);
}

void Decoder::consume(uint8_t c) {
    if(rxbufSize - rxbufPos < 2) {
        if (logCallback) {
            logCallback(LogType::RXBufferOverflow, "RX buffer overflow");
        }
        if (logCallbackEx) {
            logCallbackEx(LogInfo(ErrorCode::RXBufferOverflow, consumedCount, "RX buffer overflow"));
        }
        lastError = ErrorInfo(ErrorCode::RXBufferOverflow, consumedCount, "RX buffer overflow");
		reset();
    }
    // Adapted from https://techoverflow.net/2022/07/19/a-python-slip-decoder-using-serial_asyncio/
    if(lastCharIsEsc) {
        // This character must be either
        // SLIP_ESCEND or SLIP_ESCESC
        if(c == ESCEND) { // Literal END character
            rxbuf[rxbufPos++] = END;
        } else if(c == ESCESC) { // Literal ESC character
            rxbuf[rxbufPos++] = ESC;
        } else {
            //print(red("Encountered invalid SLIP escape sequence. Ignoring..."))
            // Ignore bad part of message
            lastError = ErrorInfo(ErrorCode::DecodeInvalidEscapeSequence, consumedCount, "Invalid escape sequence");
            reset();
        }
        lastCharIsEsc = false; // Reset state
    } else { // last char was NOT ESC
        if(c == END) { // END of message
            // Emit message
            messageCallback(rxbuf, rxbufPos);
            // Remove current message from buffer
            reset();
        } else if(c == ESC) {
            // Handle escaped character next 
            lastCharIsEsc = true;
        } else { // Any other character
            rxbuf[rxbufPos++] = c;
        }
    }
    consumedCount++;
}

Decoder::ConsumeResult Decoder::consume_ex(uint8_t c) {
    if(rxbufSize - rxbufPos < 2) {
        if (logCallback) {
            logCallback(LogType::RXBufferOverflow, "RX buffer overflow");
        }
        if (logCallbackEx) {
            logCallbackEx(LogInfo(ErrorCode::RXBufferOverflow, consumedCount, "RX buffer overflow"));
        }
        lastError = ErrorInfo(ErrorCode::RXBufferOverflow, consumedCount, "RX buffer overflow");
        reset();
        return ConsumeResult(ErrorCode::RXBufferOverflow, 1, consumedCount, "RX buffer overflow");
    }
    // Adapted from https://techoverflow.net/2022/07/19/a-python-slip-decoder-using-serial_asyncio/
    if(lastCharIsEsc) {
        // This character must be either
        // SLIP_ESCEND or SLIP_ESCESC
        if(c == ESCEND) { // Literal END character
            rxbuf[rxbufPos++] = END;
        } else if(c == ESCESC) { // Literal ESC character
            rxbuf[rxbufPos++] = ESC;
        } else {
            //print(red("Encountered invalid SLIP escape sequence. Ignoring..."))
            // Ignore bad part of message
            lastError = ErrorInfo(ErrorCode::DecodeInvalidEscapeSequence, consumedCount, "Invalid escape sequence");
            reset();
            return ConsumeResult(ErrorCode::DecodeInvalidEscapeSequence, 1, consumedCount, "Invalid escape sequence");
        }
        lastCharIsEsc = false; // Reset state
    } else { // last char was NOT ESC
        if(c == END) { // END of message
            // Emit message
            messageCallback(rxbuf, rxbufPos);
            // Remove current message from buffer
            reset();
        } else if(c == ESC) {
            // Handle escaped character next 
            lastCharIsEsc = true;
        } else { // Any other character
            rxbuf[rxbufPos++] = c;
        }
    }
    consumedCount++;
    return ConsumeResult(1);
}

Decoder::ConsumeResult Decoder::consume_chunk_ex(const uint8_t* data, size_t size, size_t chunk_size) {
    size_t consumed = 0;
    while (consumed < size) {
        size_t to_consume = std::min(chunk_size, size - consumed);
        ConsumeResult result = consume_ex(data + consumed, to_consume);
        if (result.has_error) {
            return ConsumeResult(result.error.code, consumed, result.error.position, result.error.message);
        }
        consumed += result.consumed;
    }
    return ConsumeResult(consumed);
}

void Decoder::reset() {
    rxbufPos = 0;
	lastCharIsEsc = false;
    lastError = ErrorInfo(ErrorCode::Success);
    consumedCount = 0;
}

} // namespace SLIPStream
