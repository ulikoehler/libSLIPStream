# SLIP Test Data Files

This directory contains test data files for SLIP (Serial Line Internet Protocol) encoding/decoding.

## SLIP Protocol Reference

- **END**: 0xC0
- **ESC**: 0xDB
- **ESC_END**: 0xDC
- **ESC_ESC**: 0xDD

## Test Files

### simple_frame.bin
A single simple frame containing ASCII text "hello".
- Decoded: "hello"
- Encoded: `68 65 6C 6C 6F C0`

### frame_with_escapes.bin
A frame containing special bytes that need escaping (END and ESC).
- Decoded: `C0 DB 01` (END, ESC, 0x01)
- Encoded: `DB DC DB DD 01 C0`

### multiple_frames.bin
Multiple frames in sequence.
- Decoded: ["one", "two", "three"]
- Encoded: Three SLIP frames concatenated

### empty_frame.bin
An empty frame (just the END delimiter).
- Decoded: ""
- Encoded: `C0`

### large_frame.bin
A larger frame with varied data.
- Decoded: 100 bytes of sequential data
- Encoded: SLIP-encoded with potential escapes

### incomplete_frame.bin
An incomplete frame missing the END delimiter.
- Decoded: "incomplete"
- Encoded: Missing final C0 (for error testing)

### frame_with_all_special.bin
A frame containing all special SLIP bytes.
- Decoded: `C0 DB DC DD` (all special bytes)
- Encoded: `DB DC DB DD DB DE DB DF C0`

## Usage Examples

### C++ (libSLIPStream)
```cpp
#include <fstream>
#include <vector>
#include "SLIPStream/Decoder.hpp"

std::ifstream file("test_data/simple_frame.bin", std::ios::binary);
std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), 
                          std::istreambuf_iterator<char>());
SLIPStream::Decoder decoder;
auto frames = decoder.decode(data);
```

### Rust (SLIPSpeed)
```rust
use std::fs;
use slipspeed::decode_frames;

let data = fs::read("test_data/simple_frame.bin").unwrap();
let frames = decode_frames(&data).unwrap();
```

### Python (PySLIPStream)
```python
with open("test_data/simple_frame.bin", "rb") as f:
    data = f.read()
frames = slipstream.decode(data)
```
