// CRC32 benchmarks
#include <benchmark/benchmark.h>
#include <vector>
#include <cstring>
#include "SLIPStream/CRC32.hpp"

static void BM_CRC32_Calculate_Small(benchmark::State& state) {
    std::vector<uint8_t> data(16);
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::calculate_crc32(data.data(), data.size()));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_CRC32_Calculate_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(256);
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::calculate_crc32(data.data(), data.size()));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_CRC32_Calculate_Large(benchmark::State& state) {
    std::vector<uint8_t> data(1024);
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::calculate_crc32(data.data(), data.size()));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_CRC32_Calculate_VeryLarge(benchmark::State& state) {
    std::vector<uint8_t> data(8192);
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::calculate_crc32(data.data(), data.size()));
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_CRC32_Append_Small(benchmark::State& state) {
    std::vector<uint8_t> data(32);
    std::vector<uint8_t> payload(16);
    for (auto _ : state) {
        memcpy(data.data(), payload.data(), payload.size());
        SLIPStream::append_crc32(data.data(), payload.size());
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Append_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(512);
    std::vector<uint8_t> payload(256);
    for (auto _ : state) {
        memcpy(data.data(), payload.data(), payload.size());
        SLIPStream::append_crc32(data.data(), payload.size());
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Append_Large(benchmark::State& state) {
    std::vector<uint8_t> data(2048);
    std::vector<uint8_t> payload(1024);
    for (auto _ : state) {
        memcpy(data.data(), payload.data(), payload.size());
        SLIPStream::append_crc32(data.data(), payload.size());
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Verify_Small(benchmark::State& state) {
    std::vector<uint8_t> data(32);
    std::vector<uint8_t> payload(16);
    memcpy(data.data(), payload.data(), payload.size());
    SLIPStream::append_crc32(data.data(), payload.size());
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::verify_crc32(data.data(), payload.size() + 4));
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Verify_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(512);
    std::vector<uint8_t> payload(256);
    memcpy(data.data(), payload.data(), payload.size());
    SLIPStream::append_crc32(data.data(), payload.size());
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::verify_crc32(data.data(), payload.size() + 4));
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Verify_Large(benchmark::State& state) {
    std::vector<uint8_t> data(2048);
    std::vector<uint8_t> payload(1024);
    memcpy(data.data(), payload.data(), payload.size());
    SLIPStream::append_crc32(data.data(), payload.size());
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(SLIPStream::verify_crc32(data.data(), payload.size() + 4));
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Extract_Small(benchmark::State& state) {
    std::vector<uint8_t> data(32);
    std::vector<uint8_t> payload(16);
    memcpy(data.data(), payload.data(), payload.size());
    SLIPStream::append_crc32(data.data(), payload.size());
    
    uint32_t crc;
    for (auto _ : state) {
        SLIPStream::extract_crc32(data.data(), payload.size() + 4, &crc);
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Extract_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(512);
    std::vector<uint8_t> payload(256);
    memcpy(data.data(), payload.data(), payload.size());
    SLIPStream::append_crc32(data.data(), payload.size());
    
    uint32_t crc;
    for (auto _ : state) {
        SLIPStream::extract_crc32(data.data(), payload.size() + 4, &crc);
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Extract_Large(benchmark::State& state) {
    std::vector<uint8_t> data(2048);
    std::vector<uint8_t> payload(1024);
    memcpy(data.data(), payload.data(), payload.size());
    SLIPStream::append_crc32(data.data(), payload.size());
    
    uint32_t crc;
    for (auto _ : state) {
        SLIPStream::extract_crc32(data.data(), payload.size() + 4, &crc);
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Roundtrip_Small(benchmark::State& state) {
    std::vector<uint8_t> data(32);
    std::vector<uint8_t> payload(16);
    
    for (auto _ : state) {
        memcpy(data.data(), payload.data(), payload.size());
        size_t new_len = SLIPStream::append_crc32(data.data(), payload.size());
        bool valid = SLIPStream::verify_crc32(data.data(), new_len);
        benchmark::DoNotOptimize(valid);
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Roundtrip_Medium(benchmark::State& state) {
    std::vector<uint8_t> data(512);
    std::vector<uint8_t> payload(256);
    
    for (auto _ : state) {
        memcpy(data.data(), payload.data(), payload.size());
        size_t new_len = SLIPStream::append_crc32(data.data(), payload.size());
        bool valid = SLIPStream::verify_crc32(data.data(), new_len);
        benchmark::DoNotOptimize(valid);
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

static void BM_CRC32_Roundtrip_Large(benchmark::State& state) {
    std::vector<uint8_t> data(2048);
    std::vector<uint8_t> payload(1024);
    
    for (auto _ : state) {
        memcpy(data.data(), payload.data(), payload.size());
        size_t new_len = SLIPStream::append_crc32(data.data(), payload.size());
        bool valid = SLIPStream::verify_crc32(data.data(), new_len);
        benchmark::DoNotOptimize(valid);
    }
    state.SetBytesProcessed(state.iterations() * payload.size());
}

BENCHMARK(BM_CRC32_Calculate_Small);
BENCHMARK(BM_CRC32_Calculate_Medium);
BENCHMARK(BM_CRC32_Calculate_Large);
BENCHMARK(BM_CRC32_Calculate_VeryLarge);

BENCHMARK(BM_CRC32_Append_Small);
BENCHMARK(BM_CRC32_Append_Medium);
BENCHMARK(BM_CRC32_Append_Large);

BENCHMARK(BM_CRC32_Verify_Small);
BENCHMARK(BM_CRC32_Verify_Medium);
BENCHMARK(BM_CRC32_Verify_Large);

BENCHMARK(BM_CRC32_Extract_Small);
BENCHMARK(BM_CRC32_Extract_Medium);
BENCHMARK(BM_CRC32_Extract_Large);

BENCHMARK(BM_CRC32_Roundtrip_Small);
BENCHMARK(BM_CRC32_Roundtrip_Medium);
BENCHMARK(BM_CRC32_Roundtrip_Large);
