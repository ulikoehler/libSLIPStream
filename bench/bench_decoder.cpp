// Decoder benchmarks
#include <benchmark/benchmark.h>
#include <vector>
#include "SLIPStream/Decoder.hpp"

static void BM_Decoder_Consume_Small(benchmark::State& state) {
    std::vector<uint8_t> rx_buffer(512);
    std::vector<uint8_t> data(16);
    std::vector<uint8_t> encoded(32);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    
    std::vector<std::vector<uint8_t>> received_messages;
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        received_messages.emplace_back(data, data + size);
    };
    
    std::function<void(SLIPStream::LogType, const char*)> log_callback = nullptr;
    
    SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), message_callback, log_callback);
    
    for (auto _ : state) {
        decoder.consume(encoded.data(), encoded_len);
        received_messages.clear();
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Decoder_Consume_Medium(benchmark::State& state) {
    std::vector<uint8_t> rx_buffer(1024);
    std::vector<uint8_t> data(256);
    std::vector<uint8_t> encoded(512);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    
    std::vector<std::vector<uint8_t>> received_messages;
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        received_messages.emplace_back(data, data + size);
    };
    
    std::function<void(SLIPStream::LogType, const char*)> log_callback = nullptr;
    
    SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), message_callback, log_callback);
    
    for (auto _ : state) {
        decoder.consume(encoded.data(), encoded_len);
        received_messages.clear();
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Decoder_Consume_Large(benchmark::State& state) {
    std::vector<uint8_t> rx_buffer(2048);
    std::vector<uint8_t> data(1024);
    std::vector<uint8_t> encoded(2048);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    
    std::vector<std::vector<uint8_t>> received_messages;
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        received_messages.emplace_back(data, data + size);
    };
    
    std::function<void(SLIPStream::LogType, const char*)> log_callback = nullptr;
    
    SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), message_callback, log_callback);
    
    for (auto _ : state) {
        decoder.consume(encoded.data(), encoded_len);
        received_messages.clear();
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Decoder_Consume_WithSpecialBytes(benchmark::State& state) {
    std::vector<uint8_t> rx_buffer(2048);
    std::vector<uint8_t> data(256);
    // Fill with special bytes to test worst-case decoding
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = (i % 2 == 0) ? 0xC0 : 0xDB;
    }
    std::vector<uint8_t> encoded(1024);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    
    std::vector<std::vector<uint8_t>> received_messages;
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        received_messages.emplace_back(data, data + size);
    };
    
    std::function<void(SLIPStream::LogType, const char*)> log_callback = nullptr;
    
    SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), message_callback, log_callback);
    
    for (auto _ : state) {
        decoder.consume(encoded.data(), encoded_len);
        received_messages.clear();
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

static void BM_Decoder_Consume_MultiplePackets(benchmark::State& state) {
    std::vector<uint8_t> rx_buffer(4096);
    std::vector<uint8_t> data(32);
    std::vector<uint8_t> encoded(64);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    
    std::vector<std::vector<uint8_t>> received_messages;
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        received_messages.emplace_back(data, data + size);
    };
    
    std::function<void(SLIPStream::LogType, const char*)> log_callback = nullptr;
    
    SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), message_callback, log_callback);
    
    for (auto _ : state) {
        for (int i = 0; i < 10; i++) {
            decoder.consume(encoded.data(), encoded_len);
        }
        received_messages.clear();
    }
    state.SetBytesProcessed(state.iterations() * data.size() * 10);
}

static void BM_Decoder_Reset(benchmark::State& state) {
    std::vector<uint8_t> rx_buffer(512);
    std::vector<uint8_t> data(16);
    std::vector<uint8_t> encoded(32);
    size_t encoded_len = SLIPStream::encode_packet(data.data(), data.size(), encoded.data(), encoded.size());
    
    std::vector<std::vector<uint8_t>> received_messages;
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        received_messages.emplace_back(data, data + size);
    };
    
    std::function<void(SLIPStream::LogType, const char*)> log_callback = nullptr;
    
    for (auto _ : state) {
        SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), message_callback, log_callback);
        decoder.consume(encoded.data(), encoded_len);
        decoder.reset();
        received_messages.clear();
    }
    state.SetBytesProcessed(state.iterations() * data.size());
}

BENCHMARK(BM_Decoder_Consume_Small);
BENCHMARK(BM_Decoder_Consume_Medium);
BENCHMARK(BM_Decoder_Consume_Large);
BENCHMARK(BM_Decoder_Consume_WithSpecialBytes);
BENCHMARK(BM_Decoder_Consume_MultiplePackets);
BENCHMARK(BM_Decoder_Reset);
