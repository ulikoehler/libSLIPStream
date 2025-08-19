# libSLIPStream

SLIP encoder & decoder focused on embedded systems, in C++.

This small, unopinionated library provides buffer-based helpers to encode and decode data using the SLIP (Serial Line Internet Protocol) framing. It is designed to be lightweight and safe for constrained environments: callers provide input/output buffers and the library returns explicit error codes when buffers are too small or input is malformed.

## Key symbols (namespaced)

- `#include "SLIPStream/Buffer.hpp"` — high-level buffer helpers
- `SLIPStream::encoded_length(const uint8_t* in, size_t inlen)` — compute encoded size (including final END)
- `SLIPStream::encode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen)` — encode into `out`
- `SLIPStream::decoded_length(const uint8_t* in, size_t inlen)` — compute decoded size up to first END
- `SLIPStream::decode_packet(const uint8_t* in, size_t inlen, uint8_t* out, size_t outlen)` — decode into `out`
- `SLIPStream::ENCODE_ERROR` / `SLIPStream::DECODE_ERROR` — functions return this (`SIZE_MAX`) on errors

See the header `include/SLIPStream/Buffer.hpp` for documentation and function contracts.

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

## Error handling and defensive tips

- Treat `SLIPStream::ENCODE_ERROR` and `SLIPStream::DECODE_ERROR` as fatal for the current operation and recover by discarding the partial buffer or requesting a retry.
- When reading from a stream, accumulate bytes until you observe an END (0xC0) before attempting `SLIPStream::decoded_length()` and decode. That avoids repeatedly scanning incomplete frames.
- Be careful about concurrent access: functions are buffer-based and reentrant as long as you don't share the same input/output buffers concurrently.

## Building & testing

This repository includes a small CMake test under `test/` that now builds a single test executable named `test_all` which contains all unit tests.

From the project root (out-of-source build recommended):

```sh
mkdir -p build && cd build
cmake ..
cmake --build . --target test_all
# Run the tests via CTest (this runs the test_all executable)
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

## License

See `LICENSE` for license information.

