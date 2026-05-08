# libSLIPspeed

![libSLIPspeed logo](docs/logo.png)

SLIP encoder & decoder  suitable for platforms from embedded systems to servers.

This small, unopinionated library provides buffer- and stream-based helpers to encode and decode data using the SLIP (Serial Line Internet Protocol) framing with or without CRC. It is designed to be lightweight and safe for constrained environments: callers provide input/output buffers and the library returns explicit error codes when buffers are too small or input is malformed.

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
Run on (32 X 5759 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x16)
  L1 Instruction 32 KiB (x16)
  L2 Unified 1024 KiB (x16)
  L3 Unified 98304 KiB (x2)
Load Average: 6.01, 4.61, 9.11
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** Library was built as DEBUG. Timings may be affected.
-------------------------------------------------------------------------------------------------
Benchmark                                       Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------------
BM_Buffer_EncodedLength_Small                5.92 ns         5.92 ns    112619909 bytes_per_second=2.51659Gi/s
BM_Buffer_EncodedLength_Medium               85.5 ns         85.5 ns      8311018 bytes_per_second=2.78937Gi/s
BM_Buffer_EncodedLength_Large                 319 ns          319 ns      2089336 bytes_per_second=2.99166Gi/s
BM_Buffer_Encode_Small                       8.40 ns         8.39 ns     83212517 bytes_per_second=1.77516Gi/s
BM_Buffer_Encode_Medium                       110 ns          110 ns      6394295 bytes_per_second=2.17157Gi/s
BM_Buffer_Encode_Large                        422 ns          422 ns      1666835 bytes_per_second=2.26111Gi/s
BM_Buffer_Encode_WithSpecialBytes             133 ns          133 ns      5283615 bytes_per_second=1.79578Gi/s
BM_Buffer_Encode_ASCII_Small                 8.29 ns         8.29 ns     82098556 bytes_per_second=1.79747Gi/s
BM_Buffer_Encode_ASCII_Medium                 109 ns          109 ns      6448603 bytes_per_second=2.18429Gi/s
BM_Buffer_Encode_ASCII_Large                  417 ns          417 ns      1678069 bytes_per_second=2.28499Gi/s
BM_Buffer_Decode_Small                       8.39 ns         8.39 ns     83137422 bytes_per_second=1.77676Gi/s
BM_Buffer_Decode_Medium                       134 ns          134 ns      5737978 bytes_per_second=1.78007Gi/s
BM_Buffer_Decode_Large                        522 ns          522 ns      1347194 bytes_per_second=1.82802Gi/s
BM_Buffer_Decode_WithSpecialBytes             174 ns          174 ns      4000138 bytes_per_second=1.36692Gi/s
BM_Buffer_Decode_ASCII_Small                 8.28 ns         8.28 ns     85161734 bytes_per_second=1.80038Gi/s
BM_Buffer_Decode_ASCII_Medium                 133 ns          133 ns      6061027 bytes_per_second=1.78928Gi/s
BM_Buffer_Decode_ASCII_Large                  507 ns          507 ns      1346242 bytes_per_second=1.88131Gi/s
BM_Buffer_Roundtrip_Small                    16.6 ns         16.6 ns     42455372 bytes_per_second=921.394Mi/s
BM_Buffer_Roundtrip_Medium                    225 ns          225 ns      3268368 bytes_per_second=1.0606Gi/s
BM_Buffer_Roundtrip_Large                     932 ns          931 ns       749064 bytes_per_second=1.02381Gi/s
BM_Buffer_Roundtrip_ASCII_Small              16.4 ns         16.4 ns     42211330 bytes_per_second=929.267Mi/s
BM_Buffer_Roundtrip_ASCII_Medium              215 ns          215 ns      3294096 bytes_per_second=1.10688Gi/s
BM_Buffer_Roundtrip_ASCII_Large               834 ns          834 ns       837302 bytes_per_second=1.14381Gi/s
BM_Encoder_PushPacket_Small                  56.5 ns         56.5 ns     12251054 bytes_per_second=270.083Mi/s
BM_Encoder_PushPacket_Medium                  740 ns          740 ns       956275 bytes_per_second=329.817Mi/s
BM_Encoder_PushPacket_Large                  2914 ns         2914 ns       238158 bytes_per_second=335.107Mi/s
BM_Encoder_PushPacket_WithSpecialBytes       2065 ns         2065 ns       338523 bytes_per_second=118.251Mi/s
BM_Encoder_PushPacket_ASCII_Small            56.4 ns         56.4 ns     12394255 bytes_per_second=270.483Mi/s
BM_Encoder_PushPacket_ASCII_Medium            734 ns          734 ns       952648 bytes_per_second=332.655Mi/s
BM_Encoder_PushPacket_ASCII_Large            2916 ns         2916 ns       226315 bytes_per_second=334.874Mi/s
BM_Encoder_Flush                             57.0 ns         57.0 ns     12340812 bytes_per_second=267.572Mi/s
BM_Encoder_MultiplePackets                   1026 ns         1026 ns       681138 bytes_per_second=297.35Mi/s
BM_Decoder_Consume_Small                     24.5 ns         24.5 ns     28446694 bytes_per_second=623.67Mi/s
BM_Decoder_Consume_Medium                     223 ns          223 ns      3123641 bytes_per_second=1.06905Gi/s
BM_Decoder_Consume_Large                      842 ns          842 ns       834388 bytes_per_second=1.1329Gi/s
BM_Decoder_Consume_WithSpecialBytes           456 ns          456 ns      1533738 bytes_per_second=535.669Mi/s
BM_Decoder_Consume_MultiplePackets            392 ns          392 ns      1777866 bytes_per_second=778.35Mi/s
BM_Decoder_Reset                             32.1 ns         32.1 ns     21402350 bytes_per_second=474.826Mi/s
BM_Decoder_Consume_SingleByte                 430 ns          430 ns      1619666 bytes_per_second=568.417Mi/s
BM_Decoder_Consume_4ByteChunk                 266 ns          266 ns      2655141 bytes_per_second=919.084Mi/s
BM_Decoder_Consume_AllAtOnce                  228 ns          228 ns      3009612 bytes_per_second=1.04752Gi/s
BM_CRC32_Calculate_Small                     16.7 ns         16.7 ns     33608889 bytes_per_second=915.968Mi/s
BM_CRC32_Calculate_Medium                     444 ns          444 ns      1528396 bytes_per_second=549.324Mi/s
BM_CRC32_Calculate_Large                     1834 ns         1834 ns       383570 bytes_per_second=532.546Mi/s
BM_CRC32_Calculate_VeryLarge                14799 ns        14798 ns        47604 bytes_per_second=527.946Mi/s
BM_CRC32_Append_Small                        16.5 ns         16.5 ns     42598182 bytes_per_second=923.2Mi/s
BM_CRC32_Append_Medium                        445 ns          445 ns      1579699 bytes_per_second=548.589Mi/s
BM_CRC32_Append_Large                        1839 ns         1839 ns       381032 bytes_per_second=530.986Mi/s
BM_CRC32_Verify_Small                        16.7 ns         16.7 ns     42170662 bytes_per_second=914.184Mi/s
BM_CRC32_Verify_Medium                        445 ns          445 ns      1570909 bytes_per_second=548.095Mi/s
BM_CRC32_Verify_Large                        1821 ns         1821 ns       385081 bytes_per_second=536.306Mi/s
BM_CRC32_Extract_Small                       1.20 ns         1.20 ns    586457795 bytes_per_second=12.4647Gi/s
BM_CRC32_Extract_Medium                      1.19 ns         1.19 ns    584822677 bytes_per_second=200.084Gi/s
BM_CRC32_Extract_Large                       1.20 ns         1.20 ns    586421276 bytes_per_second=797.018Gi/s
BM_CRC32_Roundtrip_Small                     33.3 ns         33.3 ns     21031787 bytes_per_second=458.201Mi/s
BM_CRC32_Roundtrip_Medium                     888 ns          888 ns       790292 bytes_per_second=274.792Mi/s
BM_CRC32_Roundtrip_Large                     3665 ns         3664 ns       191573 bytes_per_second=266.556Mi/s
```

*Note: Benchmarks were built with `-O3 -march=native` optimizations. Results will vary based on CPU architecture, compiler version, and system configuration.*

## License

See `LICENSE` for license information.

## Python Binding

The official Python binding for libSLIPspeed is now available as a separate project: **[slipspeed](https://github.com/ulikoehler/PySLIPStream)**.

slipspeed provides:
- Full parity with libSLIPspeed for SLIP encoding/decoding
- Identical CRC32 implementation (Ethernet polynomial)
- Serial and TCP connection support
- Interactive monitoring tools with ncurses UI
- Automated parity testing against libSLIPspeed

### Installation

```bash
pip install slipspeed
```

Or from source:
```bash
pip install git+https://github.com/ulikoehler/PySLIPStream.git
```

### Quick Example

```python
from slipspeed import encode_packet, decode_packet

# Encode data
data = b"Hello, World!"
encoded = encode_packet(data)

# Decode data
decoded, consumed = decode_packet(encoded)
```

For full documentation and examples, visit [slipspeed](https://github.com/ulikoehler/PySLIPStream).

