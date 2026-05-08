// Buffer API benchmarks
#include <benchmark/benchmark.h>
#include <vector>
#include <cstring>
#include "SLIPStream/Buffer.hpp"

static void BM_Buffer_EncodedLength_Small(benchmark::State& state) {
    std::vector<uint8_t> data(16);
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::encoded_length(data.data(), data.size()));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_EncodedLength_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(256);
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::encoded_length(data.data(), data.size()));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_EncodedLength_Large(benchmark::State& state) {
    std::vector<uint8_t> data(1024);
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::encoded_length(data.data(), data.size()));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Encode_Small(benchmark::State& state) {
    std::vector<uint8_t> data(16);
    std::vector<uint8_t> out(32);
    for (auto _ : state) {
        SLIPStream::encode_packet(data.data(), data.size(), out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Encode_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(256);
    std::vector<uint8_t> out(512);
    for (auto _ : state) {
        SLIPStream::encode_packet(data.data(), data.size(), out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Encode_Large(benchmark::State& state) {
    std::vector<uint8_t> data(1024);
    std::vector<uint8_t> out(2048);
    for (auto _ : state) {
        SLIPStream::encode_packet(data.data(), data.size(), out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Decode_Small(benchmark::State& state) {
    std::vector<uint8_t> data(16);
    std::vector<uint8_t> encoded(32);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    std::vector<uint8_t> out(16);
    
    for (auto _ : state) {
        SLIPStream::decode_packet(encoded.data(), encoded_len, out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Decode_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(256);
    std::vector<uint8_t> encoded(512);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    std::vector<uint8_t> out(256);
    
    for (auto _ : state) {
        SLIPStream::decode_packet(encoded.data(), encoded_len, out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Decode_Large(benchmark::State& state) {
    std::vector<uint8_t> data(1024);
    std::vector<uint8_t> encoded(2048);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    std::vector<uint8_t> out(1024);
    
    for (auto _ : state) {
        SLIPStream::decode_packet(encoded.data(), encoded_len, out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Roundtrip_Small(benchmark::State& state) {
    std::vector<uint8_t> data(16);
    std::vector<uint8_t> encoded(32);
    std::vector<uint8_t> decoded(16);
    
    for (auto _ : state) {
        size_t enc_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
        SLIPStream::decode_packet(encoded.data(), enc_len, decoded.data(), decoded.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Roundtrip_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(256);
    std::vector<uint8_t> encoded(512);
    std::vector<uint8_t> decoded(256);
    
    for (auto _ : state) {
        size_t enc_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
        SLIPStream::decode_packet(encoded.data(), enc_len, decoded.data(), decoded.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Roundtrip_Large(benchmark::State& state) {
    std::vector<uint8_t> data(1024);
    std::vector<uint8_t> encoded(2048);
    std::vector<uint8_t> decoded(1024);
    
    for (auto _ : state) {
        size_t enc_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
        SLIPStream::decode_packet(encoded.data(), enc_len, decoded.data(), decoded.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Encode_WithSpecialBytes(benchmark::State& state) {
    std::vector<uint8_t> data(256);
    // Fill with special bytes to test worst-case encoding
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = (i % 2 == 0) ? 0xC0 : 0xDB;  // END and ESC
    }
    std::vector<uint8_t> out(1024);
    
    for (auto _ : state) {
        SLIPStream::encode_packet(data.data(), data.size(), out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Buffer_Decode_WithSpecialBytes(benchmark::State& state) {
    std::vector<uint8_t> data(256);
    // Fill with special bytes to test worst-case decoding
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = (i % 2 == 0) ? 0xC0 : 0xDB;  // END and ESC
    }
    std::vector<uint8_t> encoded(1024);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    std::vector<uint8_t> out(256);
    
    for (auto _ : state) {
        SLIPStream::decode_packet(encoded.data(), encoded_len, out.data(), out.size());
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

BENCHMARK(BM_Buffer_EncodedLength_Small);
BENCHMARK(BM_Buffer_EncodedLength_Medium);
BENCHMARK(BM_Buffer_EncodedLength_Large);

BENCHMARK(BM_Buffer_Encode_Small);
BENCHMARK(BM_Buffer_Encode_Medium);
BENCHMARK(BM_Buffer_Encode_Large);
BENCHMARK(BM_Buffer_Encode_WithSpecialBytes);

BENCHMARK(BM_Buffer_Decode_Small);
BENCHMARK(BM_Buffer_Decode_Medium);
BENCHMARK(BM_Buffer_Decode_Large);
BENCHMARK(BM_Buffer_Decode_WithSpecialBytes);

BENCHMARK(BM_Buffer_Roundtrip_Small);
BENCHMARK(BM_Buffer_Roundtrip_Medium);
BENCHMARK(BM_Buffer_Roundtrip_Large);
