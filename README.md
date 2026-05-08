# libSLIPStream

![libSLIPStream logo](docs/SLIPstream-logo.png)

SLIP encoder & decoder focused on embedded systems, in C++.

This small, unopinionated library provides buffer-based helpers to encode and decode data using the SLIP (Serial Line Internet Protocol) framing. It is designed to be lightweight and safe for constrained environments: callers provide input/output buffers and the library returns explicit error codes when buffers are too small or input is malformed.

**Note:** For a compatible Rust implementation, see [SLIPSpeed](https://github.com/ulikoehler/slipspeed).

## Key symbols (namespaced)

### Buffer API
- `#include "SLIPStream/Buffer.hpp"` — high-level buffer helpers
- `SLIPStream::encoded_length(const uint8_t* in, size_t inlen)` — compute encoded size (including final END)
- `SLIPStream::encode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen)` — encode into `out`
- `SLIPStream::decoded_length(const uint8_t* in, size_t inlen)` — compute decoded size up to first END
- `SLIPStream::decode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen)` — decode into `out`
- `SLIPStream::ENCODE_ERROR` / `SLIPStream::DECODE_ERROR` — functions return this (`SIZE_MAX`) on errors

### Enhanced Error Reporting API
- `#include "SLIPStream/Error.hpp"` — error reporting system
- `#include "SLIPStream/Buffer.hpp"` — enhanced buffer functions (`*_ex`)
- `SLIPStream::encoded_length_ex(const uint8_t* in, size_t inlen)` — enhanced version with detailed errors
- `SLIPStream::encode_packet_ex(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen)` — enhanced encode with error info
- `SLIPStream::decoded_length_ex(const uint8_t* in, size_t inlen)` — enhanced decode length with error info
- `SLIPStream::decode_packet_ex(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen)` — enhanced decode with error info
- `SLIPStream::Result<T>` — template for returning values or errors
- `SLIPStream::ErrorCode` — enum with specific error types
- `SLIPStream::ErrorInfo` — struct containing error code, position, and message

### Stateful Encoder/Decoder
- `#include "SLIPStream/Encoder.hpp"` — stateful non-blocking encoder
- `SLIPStream::Encoder` — encoder class with internal buffering
- `#include "SLIPStream/Decoder.hpp"` — stateful decoder
- `SLIPStream::Decoder` — decoder class with callback-based message delivery

### CRC32 Support
- `#include "SLIPStream/CRC32.hpp"` — CRC32 calculation using Ethernet polynomial
- `SLIPStream::calculate_crc32(const uint8_t* data, size_t length)` — calculate CRC32 checksum
- `SLIPStream::calculate_crc32_with_initial(const uint8_t* data, size_t length, uint32_t initial_crc)` — calculate CRC32 with custom initial value
- `SLIPStream::append_crc32(uint8_t* data, size_t length)` — append CRC32 to data buffer (little-endian)
- `SLIPStream::extract_crc32(const uint8_t* data, size_t length, uint32_t* crc_out)` — extract CRC32 from data buffer
- `SLIPStream::verify_crc32(const uint8_t* data, size_t length)` — verify CRC32 checksum in data

CRC32 uses the Ethernet polynomial (0x04C11DB7) with initial value 0xFFFFFFFF, matching the Python implementation for full parity.

See the headers in `include/SLIPStream/` for detailed documentation and function contracts.

## Basic usage (encoding)

Typical pattern for encoding safely:

1. Call `SLIPStream::encoded_length()` to determine how many bytes the encoded packet will occupy.
2. Allocate an output buffer of at least that size (stack, static, or heap as appropriate).
3. Call `SLIPStream::encode_packet()` and check the return value.

Example:

```cpp
#include "SLIPStream/Buffer.hpp"
#include <vector>
#include <cstdio>

void encode_example() {
	const uint8_t payload[] = { 0x01, 0xC0, 0x02 }; // contains a SLIP END (0xC0)
	size_t needed = SLIPStream::encoded_length(payload, sizeof(payload));
	std::vector<uint8_t> out(needed);

	size_t written = SLIPStream::encode_packet(payload, sizeof(payload), out.data(), out.size());
	if (written == SLIPStream::ENCODE_ERROR) {
		// handle insufficient buffer or other internal error
		std::puts("encode failed: output buffer too small");
		return;
	}

	// 'written' bytes in out.data() now contain a single SLIP-framed packet (terminated by END)
}
```

Notes:
- Always use the length returned by `SLIPStream::encoded_length()` when allocating the destination buffer.
- The encoder appends a single SLIP END byte at the end of the encoded packet.

## Basic usage (decoding)

Typical pattern for decoding safely:

1. Receive a raw buffer from the transport (serial, socket, etc.). The buffer must contain at least one SLIP END byte for a full frame.
2. Call `SLIPStream::decoded_length()` to determine how many decoded bytes will result (up to the first END).
3. Allocate a buffer of that size and call `SLIPStream::decode_packet()`.

Example:

```cpp
#include "SLIPStream/Buffer.hpp"
#include <vector>
#include <cstdio>

void decode_example(const uint8_t* rx, size_t rxlen) {
	size_t decoded_needed = SLIPStream::decoded_length(rx, rxlen);
	if (decoded_needed == SLIPStream::DECODE_ERROR) {
		std::puts("decode length failed: malformed input or missing END");
		return;
	}

	std::vector<uint8_t> out(decoded_needed);
	size_t got = SLIPStream::decode_packet(rx, rxlen, out.data(), out.size());
	if (got == SLIPStream::DECODE_ERROR) {
		std::puts("decode failed: output buffer too small or malformed input");
		return;
	}

	// 'got' bytes in out.data() are the decoded payload
}
```

Notes:
- `SLIPStream::decoded_length()` stops at the first SLIP END; if no END is present it returns `SLIPStream::DECODE_ERROR`.
- `SLIPStream::decode_packet()` returns `SLIPStream::DECODE_ERROR` for malformed escape sequences or if the provided output buffer is too small.

## Enhanced error reporting

The library provides enhanced error reporting functions that return detailed error information including error codes, positions, and descriptive messages. Use the `*_ex` versions of functions for better debugging and error handling.

### Enhanced encoding with error reporting

```cpp
#include "SLIPStream/Buffer.hpp"
#include "SLIPStream/Error.hpp"
#include <vector>
#include <cstdio>

void encode_with_detailed_errors() {
    const uint8_t payload[] = { 0x01, 0xC0, 0x02 };
    std::vector<uint8_t> out(256);
    
    // Use enhanced version for detailed error information
    SLIPStream::Result<size_t> result = SLIPStream::encode_packet_ex(
        payload, sizeof(payload), out.data(), out.size()
    );
    
    if (result.is_error()) {
        printf("Encoding failed: %s at position %zu\n", 
               result.error.message, result.error.position);
        
        // Handle specific error types
        switch (result.error.code) {
            case SLIPStream::ErrorCode::EncodeBufferTooSmall:
                printf("Output buffer too small\n");
                break;
            case SLIPStream::ErrorCode::EncodeInternalError:
                printf("Internal encoding error\n");
                break;
            default:
                printf("Unknown error\n");
        }
        return;
    }
    
    printf("Successfully encoded %zu bytes\n", result.value);
}
```

### Enhanced decoding with error reporting

```cpp
#include "SLIPStream/Buffer.hpp"
#include "SLIPStream/Error.hpp"
#include <vector>
#include <cstdio>

void decode_with_detailed_errors(const uint8_t* rx, size_t rxlen) {
    std::vector<uint8_t> out(256);
    
    // Use enhanced version for detailed error information
    SLIPStream::Result<size_t> result = SLIPStream::decode_packet_ex(
        rx, rxlen, out.data(), out.size()
    );
    
    if (result.is_error()) {
        printf("Decoding failed: %s at position %zu\n", 
               result.error.message, result.error.position);
        
        // Handle specific error types
        switch (result.error.code) {
            case SLIPStream::ErrorCode::DecodeNoEndMarker:
                printf("No END marker found in input data\n");
                break;
            case SLIPStream::ErrorCode::DecodeInvalidEscapeSequence:
                printf("Invalid escape sequence detected\n");
                break;
            case SLIPStream::ErrorCode::DecodeTruncatedEscape:
                printf("Truncated escape sequence at end of input\n");
                break;
            case SLIPStream::ErrorCode::DecodeBufferTooSmall:
                printf("Output buffer too small for decoded data\n");
                break;
            default:
                printf("Unknown error\n");
        }
        return;
    }
    
    printf("Successfully decoded %zu bytes\n", result.value);
}
```

### Error code reference

The `ErrorCode` enum provides specific error types:

- `Success` - Operation completed successfully
- `EncodeBufferTooSmall` - Output buffer is too small to hold encoded data
- `EncodeInternalError` - Internal error during encoding
- `DecodeNoEndMarker` - No END marker (0xC0) found in input data
- `DecodeInvalidEscapeSequence` - Invalid escape sequence: ESC not followed by ESCEND or ESCESC
- `DecodeTruncatedEscape` - Truncated escape sequence at end of input
- `DecodeBufferTooSmall` - Output buffer too small for decoded data
- `RXBufferOverflow` - RX buffer overflow during decoding
- `UnknownError` - Unknown error occurred

## Stateful Encoder usage

For non-blocking, streaming scenarios where you need to encode data incrementally, use the `Encoder` class with internal buffering.

```cpp
#include "SLIPStream/Encoder.hpp"
#include <vector>
#include <cstdio>

void encoder_example() {
    // Create encoder with output callback
    std::vector<uint8_t> output_buffer;
    auto output_fn = [&output_buffer](uint8_t byte) -> SLIPStream::WriteStatus {
        output_buffer.push_back(byte);
        return SLIPStream::WriteStatus::Ok;
    };
    
    SLIPStream::Encoder encoder(output_fn, 256, 64); // 256 byte buffer, 64 byte chunk size
    
    // Encode and send a packet
    const uint8_t data[] = {0x01, 0x02, 0x03};
    auto [status, consumed] = encoder.pushPacket(data, sizeof(data));
    
    if (status == SLIPStream::WriteStatus::Ok) {
        printf("Successfully encoded and sent %zu bytes\n", consumed);
    } else if (status == SLIPStream::WriteStatus::RetryLater) {
        printf("Output would block, try again later\n");
    } else {
        printf("Output error occurred\n");
    }
    
    // Flush any remaining data
    encoder.flush();
}
```

### Enhanced Encoder with error reporting

```cpp
#include "SLIPStream/Encoder.hpp"
#include <vector>
#include <cstdio>

void encoder_with_error_reporting() {
    std::vector<uint8_t> output_buffer;
    auto output_fn = [&output_buffer](uint8_t byte) -> SLIPStream::WriteStatus {
        output_buffer.push_back(byte);
        return SLIPStream::WriteStatus::Ok;
    };
    
    SLIPStream::Encoder encoder(output_fn, 256, 64);
    
    // Use enhanced pushPacket for detailed error information
    const uint8_t data[] = {0x01, 0xC0, 0x02};
    SLIPStream::Encoder::PushPacketResult result = encoder.pushPacket_ex(data, sizeof(data));
    
    if (result.is_error()) {
        printf("Encoder error: %s\n", result.error.message);
        return;
    }
    
    printf("Successfully encoded %zu bytes\n", result.consumed);
    
    // Use enhanced flush for detailed error information
    SLIPStream::WriteResult flush_result = encoder.flush_ex();
    if (flush_result.is_error()) {
        printf("Flush error: %s\n", flush_result.error.message);
    }
}
```

## Stateful Decoder usage

For streaming scenarios where you receive data incrementally, use the `Decoder` class with callback-based message delivery.

```cpp
#include "SLIPStream/Decoder.hpp"
#include <vector>
#include <cstdio>

void decoder_example() {
    std::vector<uint8_t> rx_buffer(512);
    std::vector<std::vector<uint8_t>> received_messages;
    
    // Set up message callback
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        std::vector<uint8_t> msg(data, data + size);
        received_messages.push_back(msg);
        printf("Received message: %zu bytes\n", size);
    };
    
    // Set up log callback for errors
    auto log_callback = [](SLIPStream::LogType type, const char* message) {
        if (type == SLIPStream::LogType::RXBufferOverflow) {
            printf("RX buffer overflow: %s\n", message);
        }
    };
    
    SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), 
                                message_callback, log_callback);
    
    // Feed data to the decoder
    const uint8_t incoming_data[] = {0x01, 0x02, 0xC0}; // 0xC0 is END
    decoder.consume(incoming_data, sizeof(incoming_data));
    
    printf("Total messages received: %zu\n", received_messages.size());
}
```

### Enhanced Decoder with error reporting

```cpp
#include "SLIPStream/Decoder.hpp"
#include <vector>
#include <cstdio>

void decoder_with_error_reporting() {
    std::vector<uint8_t> rx_buffer(512);
    std::vector<std::vector<uint8_t>> received_messages;
    std::vector<SLIPStream::LogInfo> log_entries;
    
    auto message_callback = [&received_messages](uint8_t* data, size_t size) {
        std::vector<uint8_t> msg(data, data + size);
        received_messages.push_back(msg);
    };
    
    // Use enhanced log callback with detailed error information
    auto log_callback_ex = [&log_entries](SLIPStream::LogInfo info) {
        log_entries.push_back(info);
        printf("Decoder log: %s (code: %d, position: %zu)\n", 
               info.message, static_cast<int>(info.error.code), info.error.position);
    };
    
    SLIPStream::Decoder decoder(rx_buffer.data(), rx_buffer.size(), 
                                message_callback, log_callback_ex);
    
    // Use enhanced consume for detailed error information
    const uint8_t incoming_data[] = {0xDB, 0xFF, 0xC0}; // Invalid escape sequence
    SLIPStream::Decoder::ConsumeResult result = decoder.consume_ex(
        incoming_data, sizeof(incoming_data)
    );
    
    if (result.has_error) {
        printf("Decoder error: %s at position %zu\n", 
               result.error.message, result.error.position);
    }
    
    // Get last error information
    SLIPStream::ErrorInfo last_error = decoder.getLastError();
    if (last_error.code != SLIPStream::ErrorCode::Success) {
        printf("Last error: %s\n", last_error.message);
    }
}
```

## CRC32 usage

The C++ library provides a pure C++ implementation of CRC32 using the Ethernet polynomial (0x04C11DB7), matching the Python implementation for full parity.

### Basic CRC32 calculation

```cpp
#include "SLIPStream/CRC32.hpp"
#include <vector>
#include <cstdio>

void crc32_example() {
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    uint32_t crc = SLIPStream::calculate_crc32(data, sizeof(data));
    
    printf("CRC32: 0x%08X\n", crc);
}
```

### Appending and verifying CRC32

```cpp
#include "SLIPStream/CRC32.hpp"
#include <vector>
#include <cstdio>

void crc32_append_verify() {
    std::vector<uint8_t> data(256);  // Allocate space for data + CRC
    const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    
    // Copy payload
    memcpy(data.data(), payload, sizeof(payload));
    
    // Append CRC32 in little-endian format
    size_t new_length = SLIPStream::append_crc32(data.data(), sizeof(payload));
    printf("Data with CRC: %zu bytes\n", new_length);  // 4 data + 4 CRC = 8 bytes
    
    // Verify CRC32
    bool valid = SLIPStream::verify_crc32(data.data(), new_length);
    printf("CRC valid: %s\n", valid ? "yes" : "no");
}
```

### Extracting CRC32

```cpp
#include "SLIPStream/CRC32.hpp"
#include <vector>
#include <cstdio>

void crc32_extract() {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x78, 0x56, 0x34, 0x12};  // Data + CRC
    
    uint32_t extracted_crc;
    size_t data_length = SLIPStream::extract_crc32(data.data(), data.size(), &extracted_crc);
    
    if (data_length > 0) {
        printf("Payload length: %zu bytes\n", data_length);
        printf("Extracted CRC: 0x%08X\n", extracted_crc);
    }
}
```

### Incremental CRC32 calculation

```cpp
#include "SLIPStream/CRC32.hpp"
#include <vector>
#include <cstdio>

void crc32_incremental() {
    const uint8_t part1[] = {0x01, 0x02};
    const uint8_t part2[] = {0x03, 0x04};
    
    // Calculate CRC of first part
    uint32_t crc = SLIPStream::calculate_crc32(part1, sizeof(part1));
    
    // Continue with second part using previous CRC as initial value
    crc = SLIPStream::calculate_crc32_with_initial(part2, sizeof(part2), crc);
    
    printf("Incremental CRC32: 0x%08X\n", crc);
}
```

### Python/C++ parity

The C++ CRC32 implementation is fully compatible with the Python implementation:

```cpp
// C++
const uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
uint32_t crc = SLIPStream::calculate_crc32(data, sizeof(data));
// Result: 0x8d87dbf0
```

```python
# Python
from slipstream.crc import calculate_crc32
crc = calculate_crc32(b"Hello, World!")
# Result: 0x8d87dbf0
```

Both implementations use:
- Polynomial: Ethernet (0x04C11DB7)
- Initial value: 0xFFFFFFFF
- Bit order: MSB-first
- Byte order: Little-endian for storage

## Edge case handling

### Handling buffer overflow

```cpp
#include "SLIPStream/Buffer.hpp"
#include "SLIPStream/Error.hpp"
#include <vector>
#include <cstdio>

void handle_buffer_overflow() {
    const uint8_t payload[] = {0x01, 0x02, 0x03};
    std::vector<uint8_t> out(2); // Too small!
    
    SLIPStream::Result<size_t> result = SLIPStream::encode_packet_ex(
        payload, sizeof(payload), out.data(), out.size()
    );
    
    if (result.is_error() && 
        result.error.code == SLIPStream::ErrorCode::EncodeBufferTooSmall) {
        // Calculate required size and retry
        size_t needed = SLIPStream::encoded_length(payload, sizeof(payload));
        out.resize(needed);
        result = SLIPStream::encode_packet_ex(payload, sizeof(payload), 
                                                out.data(), out.size());
        
        if (result.is_success()) {
            printf("Retried with correct buffer size: %zu bytes\n", result.value);
        }
    }
}
```

### Handling malformed escape sequences

```cpp
#include "SLIPStream/Buffer.hpp"
#include "SLIPStream/Error.hpp"
#include <vector>
#include <cstdio>

void handle_malformed_escapes() {
    // Data with invalid escape sequence (0xDB followed by 0xFF instead of 0xDC or 0xDD)
    const uint8_t malformed_data[] = {0x01, 0xDB, 0xFF, 0xC0};
    std::vector<uint8_t> out(256);
    
    SLIPStream::Result<size_t> result = SLIPStream::decode_packet_ex(
        malformed_data, sizeof(malformed_data), out.data(), out.size()
    );
    
    if (result.is_error()) {
        switch (result.error.code) {
            case SLIPStream::ErrorCode::DecodeInvalidEscapeSequence:
                printf("Invalid escape at byte %zu\n", result.error.position);
                // Skip past the invalid sequence and retry
                if (result.error.position + 1 < sizeof(malformed_data)) {
                    result = SLIPStream::decode_packet_ex(
                        malformed_data + result.error.position + 1,
                        sizeof(malformed_data) - result.error.position - 1,
                        out.data(), out.size()
                    );
                }
                break;
            default:
                printf("Other error: %s\n", result.error.message);
                break;
        }
    }
}
```

### Handling missing END markers

```cpp
#include "SLIPStream/Buffer.hpp"
#include "SLIPStream/Error.hpp"
#include <vector>
#include <cstdio>

void handle_missing_end_marker() {
    // Data without END marker
    const uint8_t incomplete_data[] = {0x01, 0x02, 0x03};
    std::vector<uint8_t> out(256);
    
    SLIPStream::Result<size_t> result = SLIPStream::decode_packet_ex(
        incomplete_data, sizeof(incomplete_data), out.data(), out.size()
    );
    
    if (result.is_error() && 
        result.error.code == SLIPStream::ErrorCode::DecodeNoEndMarker) {
        printf("Incomplete frame: no END marker found\n");
        printf("Accumulated %zu bytes, waiting for more data\n", 
               sizeof(incomplete_data));
        // In a real application, you would accumulate more data before retrying
    }
}
```

### Testing all byte values

```cpp
#include "SLIPStream/Buffer.hpp"
#include <vector>
#include <cstdio>

void test_all_byte_values() {
    // Test round-trip with all possible byte values
    std::vector<uint8_t> all_bytes(256);
    for (int i = 0; i < 256; i++) {
        all_bytes[i] = static_cast<uint8_t>(i);
    }
    
    size_t enc_len = SLIPStream::encoded_length(all_bytes.data(), all_bytes.size());
    std::vector<uint8_t> encoded(enc_len);
    size_t written = SLIPStream::encode_packet(all_bytes.data(), all_bytes.size(), 
                                              encoded.data(), encoded.size());
    
    if (written == SLIPStream::ENCODE_ERROR) {
        printf("Encoding failed\n");
        return;
    }
    
    size_t dec_len = SLIPStream::decoded_length(encoded.data(), encoded.size());
    if (dec_len == SLIPStream::DECODE_ERROR) {
        printf("Decoding length failed\n");
        return;
    }
    
    std::vector<uint8_t> decoded(dec_len);
    size_t dec_written = SLIPStream::decode_packet(encoded.data(), encoded.size(), 
                                                  decoded.data(), decoded.size());
    
    if (dec_written == SLIPStream::DECODE_ERROR) {
        printf("Decoding failed\n");
        return;
    }
    
    // Verify round-trip
    if (all_bytes == decoded) {
        printf("Successfully round-tripped all 256 byte values\n");
    } else {
        printf("Round-trip failed!\n");
    }
}
```

## Error handling and defensive tips

- Treat `SLIPStream::ENCODE_ERROR` and `SLIPStream::DECODE_ERROR` as fatal for the current operation and recover by discarding the partial buffer or requesting a retry.
- When reading from a stream, accumulate bytes until you observe an END (0xC0) before attempting `SLIPStream::decoded_length()` and decode. That avoids repeatedly scanning incomplete frames.
- Be careful about concurrent access: functions are buffer-based and reentrant as long as you don't share the same input/output buffers concurrently.

## Building & testing

This repository includes a standalone CMake test project under `test/` that builds a single test executable named `test_all` containing all unit tests.

The top-level `CMakeLists.txt` is an ESP-IDF component file and is not intended for standalone test builds. Use the `test/` directory for normal C++ test compilation.

From the `test/` directory (out-of-source build recommended):

```sh
cd test
mkdir -p build && cd build
cmake -DENABLE_COVERAGE=ON ..
cmake --build .
ctest -V
```

You can also run the test binary directly to list or execute GoogleTest cases and get more verbose output:

```sh
# list registered test suites/cases
./test/test_all --gtest_list_tests
# run the tests directly
./test/test_all
```

If you prefer CTest to discover and report individual GoogleTest cases, enable GoogleTest discovery in `test/CMakeLists.txt` by adding the following (optional):

```cmake
include(GoogleTest)
gtest_discover_tests(test_all)
```

If you'd like, I can enable `gtest_discover_tests(test_all)` in `test/CMakeLists.txt` so `ctest -V` lists each test case separately.

Adjust the commands for your environment (toolchain, cross-compile, or embedded workflow).

## Benchmarking

This repository includes benchmarks for measuring the performance of the SLIPStream library components using Google Benchmark.

### Building benchmarks

Benchmarks require Google Benchmark to be installed. On Ubuntu/Debian:

```sh
sudo apt-get install libbenchmark-dev
```

On other systems, you may need to build Google Benchmark from source:
```sh
git clone https://github.com/google/benchmark.git
cd benchmark
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
make -j
sudo make install
```

From the `bench/` directory:

```sh
cd bench
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### Running benchmarks

Run all benchmarks:

```sh
./bench_all
```

Run specific benchmarks:

```sh
./bench_all --benchmark_filter=Buffer
./bench_all --benchmark_filter=CRC32
./bench_all --benchmark_filter=Encoder
./bench_all --benchmark_filter=Decoder
```

Benchmark categories:
- **Buffer API**: Tests performance of encode/decode operations for different data sizes
- **Encoder**: Tests performance of the stateful encoder with various packet sizes
- **Decoder**: Tests performance of the stateful decoder with various packet sizes
- **CRC32**: Tests performance of CRC32 calculation, append, verify, and extract operations

For more benchmark options, see Google Benchmark documentation or run:

```sh
./bench_all --help
```

### Sample benchmark results

Benchmark results from AMD Ryzen 9 7950X3D 16-Core Processor, Linux 6.8.0-106-generic, Ubuntu 24.04:

```
Benchmark                                       Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------------
BM_Buffer_EncodedLength_Small                5.69 ns         5.69 ns    119060637 bytes_per_second=2.62097Gi/s
BM_Buffer_EncodedLength_Medium               84.5 ns         84.5 ns      8313243 bytes_per_second=2.82081Gi/s
BM_Buffer_EncodedLength_Large                 424 ns          424 ns      1665703 bytes_per_second=2.24944Gi/s
BM_Buffer_Encode_Small                       7.28 ns         7.28 ns     96095747 bytes_per_second=2.04728Gi/s
BM_Buffer_Encode_Medium                       110 ns          110 ns      6470204 bytes_per_second=2.17142Gi/s
BM_Buffer_Encode_Large                        423 ns          423 ns      1661689 bytes_per_second=2.25532Gi/s
BM_Buffer_Encode_WithSpecialBytes             131 ns          131 ns      5328903 bytes_per_second=1.81814Gi/s
BM_Buffer_Decode_Small                       7.84 ns         7.84 ns     89415458 bytes_per_second=1.90046Gi/s
BM_Buffer_Decode_Medium                       109 ns          109 ns      6465651 bytes_per_second=2.19274Gi/s
BM_Buffer_Decode_Large                        421 ns          421 ns      1671146 bytes_per_second=2.26688Gi/s
BM_Buffer_Decode_WithSpecialBytes             176 ns          176 ns      3976291 bytes_per_second=1.35201Gi/s
BM_Buffer_Roundtrip_Small                    14.4 ns         14.4 ns     48850354 bytes_per_second=1.03349Gi/s
BM_Buffer_Roundtrip_Medium                    215 ns          215 ns      3303080 bytes_per_second=1.11121Gi/s
BM_Buffer_Roundtrip_Large                     842 ns          842 ns       814488 bytes_per_second=1.13224Gi/s
BM_Encoder_PushPacket_Small                  57.2 ns         57.2 ns     12290136 bytes_per_second=266.868Mi/s
BM_Encoder_PushPacket_Medium                  745 ns          745 ns       944644 bytes_per_second=327.538Mi/s
BM_Encoder_PushPacket_Large                  2958 ns         2957 ns       238001 bytes_per_second=330.254Mi/s
BM_Encoder_PushPacket_WithSpecialBytes       2077 ns         2076 ns       337352 bytes_per_second=117.577Mi/s
BM_Encoder_Flush                             57.3 ns         57.3 ns     12219550 bytes_per_second=266.393Mi/s
BM_Encoder_MultiplePackets                   1041 ns         1041 ns       667080 bytes_per_second=293.281Mi/s
BM_Decoder_Consume_Small                     35.6 ns         35.6 ns     27015629 bytes_per_second=428.906Mi/s
BM_Decoder_Consume_Medium                     228 ns          228 ns      3053442 bytes_per_second=1.04574Gi/s
BM_Decoder_Consume_Large                      856 ns          856 ns       825243 bytes_per_second=1.11368Gi/s
BM_Decoder_Consume_WithSpecialBytes           461 ns          461 ns      1560501 bytes_per_second=529.638Mi/s
BM_Decoder_Consume_MultiplePackets            404 ns          404 ns      1710928 bytes_per_second=754.94Mi/s
BM_Decoder_Reset                             33.4 ns         33.4 ns     20475418 bytes_per_second=457.34Mi/s
BM_CRC32_Calculate_Small                     14.5 ns         14.5 ns     45484019 bytes_per_second=1.02415Gi/s
BM_CRC32_Calculate_Medium                     445 ns          445 ns      1566405 bytes_per_second=548.854Mi/s
BM_CRC32_Calculate_Large                     1837 ns         1837 ns       378422 bytes_per_second=531.528Mi/s
BM_CRC32_Calculate_VeryLarge                14829 ns        14825 ns        47462 bytes_per_second=526.988Mi/s
BM_CRC32_Append_Small                        16.5 ns         16.5 ns     42686782 bytes_per_second=924.432Mi/s
BM_CRC32_Append_Medium                        447 ns          447 ns      1567342 bytes_per_second=546.346Mi/s
BM_CRC32_Append_Large                        1849 ns         1849 ns       376519 bytes_per_second=528.088Mi/s
BM_CRC32_Verify_Small                        16.3 ns         16.3 ns     43229016 bytes_per_second=938.668Mi/s
BM_CRC32_Verify_Medium                        448 ns          448 ns      1561968 bytes_per_second=545.223Mi/s
BM_CRC32_Verify_Large                        1826 ns         1826 ns       382229 bytes_per_second=534.696Mi/s
BM_CRC32_Extract_Small                       1.21 ns         1.21 ns    583332410 bytes_per_second=12.3456Gi/s
BM_CRC32_Extract_Medium                      1.20 ns         1.20 ns    583280255 bytes_per_second=198.77Gi/s
BM_CRC32_Extract_Large                       1.01 ns         1.01 ns    651876046 bytes_per_second=939.825Gi/s
BM_CRC32_Roundtrip_Small                     34.8 ns         34.8 ns     20072957 bytes_per_second=438.079Mi/s
BM_CRC32_Roundtrip_Medium                     891 ns          891 ns       778304 bytes_per_second=274.063Mi/s
BM_CRC32_Roundtrip_Large                     3667 ns         3667 ns       190517 bytes_per_second=266.343Mi/s
```

*Note: Benchmarks were built with `-O3 -march=native` optimizations. Results will vary based on CPU architecture, compiler version, and system configuration.*

## License

See `LICENSE` for license information.

## Python Binding

The official Python binding for libSLIPStream is now available as a separate project: **[PySLIPStream](https://github.com/ulikoehler/PySLIPStream)**.

PySLIPStream provides:
- Full parity with libSLIPStream for SLIP encoding/decoding
- Identical CRC32 implementation (Ethernet polynomial)
- Serial and TCP connection support
- Interactive monitoring tools with ncurses UI
- Automated parity testing against libSLIPStream

### Installation

```bash
pip install pyslipstream
```

Or from source:
```bash
pip install git+https://github.com/ulikoehler/PySLIPStream.git
```

### Quick Example

```python
from slipstream import encode_packet, decode_packet

# Encode data
data = b"Hello, World!"
encoded = encode_packet(data)

# Decode data
decoded, consumed = decode_packet(encoded)
```

For full documentation and examples, visit [PySLIPStream](https://github.com/ulikoehler/PySLIPStream).

