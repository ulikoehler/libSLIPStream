#include "SLIPStream/SLIPStreamEncoder.hpp"

SLIPStreamEncoder::SLIPStreamEncoder(SLIPOutputFn outputFn, size_t txBufferSize, size_t maxSendChunk)
    : outputFn(std::move(outputFn)), txBuf(txBufferSize), txHead(0), txTail(0), txSize(0), maxSendChunk(maxSendChunk), endPending(false) {}

bool SLIPStreamEncoder::queueByte(uint8_t b) {
    if (txSize >= txBuf.size()) return false; // full
    txBuf[txTail] = b;
    txTail = (txTail + 1) % txBuf.size();
    txSize++;
    return true;
}

bool SLIPStreamEncoder::dequeueByte(uint8_t& b) {
    if (txSize == 0) return false;
    b = txBuf[txHead];
    txHead = (txHead + 1) % txBuf.size();
    txSize--;
    return true;
}

SLIPWriteStatus SLIPStreamEncoder::flush() {
    size_t sent = 0;
    while (txSize > 0 && sent < maxSendChunk) {
        uint8_t b;
        // Peek without removing
        b = txBuf[txHead];
        SLIPWriteStatus st = outputFn(b);
        if (st == SLIPWriteStatus::Ok) {
            // Actually remove
            (void)dequeueByte(b);
            sent++;
            continue;
        } else if (st == SLIPWriteStatus::RetryLater) {
            return SLIPWriteStatus::RetryLater;
        } else {
            return SLIPWriteStatus::Error;
        }
    }
    return SLIPWriteStatus::Ok;
}

SLIPWriteStatus SLIPStreamEncoder::ensureFree(size_t n) {
    if (txBuf.size() - txSize >= n) return SLIPWriteStatus::Ok;
    // Try to flush some bytes
    SLIPWriteStatus st = flush();
    if (st != SLIPWriteStatus::Ok) return st;
    return (txBuf.size() - txSize >= n) ? SLIPWriteStatus::Ok : SLIPWriteStatus::RetryLater;
}

SLIPWriteStatus SLIPStreamEncoder::encodeOne(const uint8_t* data, size_t size, size_t& consumed) {
    consumed = 0;
    if (size == 0) return SLIPWriteStatus::Ok;
    uint8_t c = *data;
    if (c == SLIP_END) {
        // Need two bytes: ESC, ESCEND
        SLIPWriteStatus st = ensureFree(2);
        if (st != SLIPWriteStatus::Ok) return st;
        if (!queueByte(SLIP_ESC)) return SLIPWriteStatus::RetryLater;
        if (!queueByte(SLIP_ESCEND)) return SLIPWriteStatus::RetryLater;
    } else if (c == SLIP_ESC) {
        // Need two bytes: ESC, ESCESC
        SLIPWriteStatus st = ensureFree(2);
        if (st != SLIPWriteStatus::Ok) return st;
        if (!queueByte(SLIP_ESC)) return SLIPWriteStatus::RetryLater;
        if (!queueByte(SLIP_ESCESC)) return SLIPWriteStatus::RetryLater;
    } else {
        SLIPWriteStatus st = ensureFree(1);
        if (st != SLIPWriteStatus::Ok) return st;
        if (!queueByte(c)) return SLIPWriteStatus::RetryLater;
    }
    consumed = 1;
    return SLIPWriteStatus::Ok;
}

std::pair<SLIPWriteStatus, size_t> SLIPStreamEncoder::pushPacket(const uint8_t* data, size_t size) {
    size_t consumed = 0;
    // First, try to send any already queued bytes for fairness
    SLIPWriteStatus st = flush();
    if (st == SLIPWriteStatus::Error) return {st, consumed};
    if (st == SLIPWriteStatus::RetryLater) return {st, consumed};

    // If we were in the middle of final END from previous call, attempt it
    if (endPending) {
        // Ensure space then queue END
        st = ensureFree(1);
        if (st != SLIPWriteStatus::Ok) return {st, consumed};
        queueByte(SLIP_END);
        endPending = false;
        // Try to send a bit immediately to reduce latency
        st = flush();
        if (st != SLIPWriteStatus::Ok) return {st, consumed};
    }

    // Encode payload bytes
    while (consumed < size) {
        size_t c = 0;
        st = encodeOne(data + consumed, size - consumed, c);
        if (st != SLIPWriteStatus::Ok) return {st, consumed};
        consumed += c;
        // Opportunistic small flush to respect maxSendChunk pacing
        st = flush();
        if (st == SLIPWriteStatus::Error) return {st, consumed};
        if (st == SLIPWriteStatus::RetryLater) return {st, consumed};
    }

    // Append END terminator for the packet
    st = ensureFree(1);
    if (st != SLIPWriteStatus::Ok) {
        if (st == SLIPWriteStatus::RetryLater) {
            endPending = true; // Remember to append END on next call
        }
        return {st, consumed};
    }
    queueByte(SLIP_END);

    // Final flush attempt
    st = flush();
    if (st != SLIPWriteStatus::Ok) return {st, consumed};
    return {SLIPWriteStatus::Ok, consumed};
}