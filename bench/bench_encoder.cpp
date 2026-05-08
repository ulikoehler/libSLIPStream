// Encoder benchmarks
#include <benchmark/benchmark.h>
#include <vector>
#include "SLIPStream/Encoder.hpp"

static void BM_Encoder_PushPacket_Small(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(1024);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 1024, 64);
    std::vector<uint8_t> data(16);
    
    for (auto _ : state) {
        encoder.pushPacket(data.data(), data.size());
        output_index = 0;  // Reset for next iteration
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Encoder_PushPacket_Medium(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(1024);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 1024, 64);
    std::vector<uint8_t> data(256);
    
    for (auto _ : state) {
        encoder.pushPacket(data.data(), data.size());
        output_index = 0;
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Encoder_PushPacket_Large(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(4096);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 4096, 256);
    std::vector<uint8_t> data(1024);
    
    for (auto _ : state) {
        encoder.pushPacket(data.data(), data.size());
        output_index = 0;
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Encoder_Flush(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(1024);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 1024, 64);
    std::vector<uint8_t> data(16);
    encoder.pushPacket(data.data(), data.size());
    
    for (auto _ : state) {
        encoder.flush();
        output_index = 0;
        encoder.pushPacket(data.data(), data.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Encoder_PushPacket_WithSpecialBytes(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(2048);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 2048, 64);
    std::vector<uint8_t> data(256);
    // Fill with special bytes to test worst-case encoding
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = (i % 2 == 0) ? 0xC0 : 0xDB;
    }
    
    for (auto _ : state) {
        encoder.pushPacket(data.data(), data.size());
        output_index = 0;
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Encoder_MultiplePackets(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(4096);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 4096, 64);
    std::vector<uint8_t> data(32);
    
    for (auto _ : state) {
        for (int i = 0; i < 10; i++) {
            encoder.pushPacket(data.data(), data.size());
        }
        output_index = 0;
    }
    state.SetBytesProcessed(state.iterations() * data.size() * 10);
}

static void BM_Encoder_PushPacket_ASCII_Small(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(1024);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 1024, 64);
    std::vector<uint8_t> data(16);
    // Fill with printable ASCII characters (0x20-0x7E)
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = 0x20 + (i % (0x7E - 0x20 + 1));
    }
    
    for (auto _ : state) {
        encoder.pushPacket(data.data(), data.size());
        output_index = 0;
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Encoder_PushPacket_ASCII_Medium(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(1024);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 1024, 64);
    std::vector<uint8_t> data(256);
    // Fill with printable ASCII characters (0x20-0x7E)
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = 0x20 + (i % (0x7E - 0x20 + 1));
    }
    
    for (auto _ : state) {
        encoder.pushPacket(data.data(), data.size());
        output_index = 0;
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Encoder_PushPacket_ASCII_Large(benchmark::State& state) {
    std::vector<uint8_t> output_buffer(4096);
    size_t output_index = 0;
    
    auto output_fn = [&output_buffer, &output_index](uint8_t byte) -> SLIPStream::WriteStatus {
        if (output_index < output_buffer.size()) {
            output_buffer[output_index++] = byte;
            return SLIPStream::WriteStatus::Ok;
        }
        return SLIPStream::WriteStatus::RetryLater;
    };
    
    SLIPStream::Encoder encoder(output_fn, 4096, 256);
    std::vector<uint8_t> data(1024);
    // Fill with printable ASCII characters (0x20-0x7E)
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = 0x20 + (i % (0x7E - 0x20 + 1));
    }
    
    for (auto _ : state) {
        encoder.pushPacket(data.data(), data.size());
        output_index = 0;
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

BENCHMARK(BM_Encoder_PushPacket_Small);
BENCHMARK(BM_Encoder_PushPacket_Medium);
BENCHMARK(BM_Encoder_PushPacket_Large);
BENCHMARK(BM_Encoder_PushPacket_WithSpecialBytes);
BENCHMARK(BM_Encoder_PushPacket_ASCII_Small);
BENCHMARK(BM_Encoder_PushPacket_ASCII_Medium);
BENCHMARK(BM_Encoder_PushPacket_ASCII_Large);
BENCHMARK(BM_Encoder_Flush);
BENCHMARK(BM_Encoder_MultiplePackets);
