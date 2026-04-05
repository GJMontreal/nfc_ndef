# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Purpose

A minimal embedded C library for ST25DVxx NFC tags. It reads and writes NDEF messages over I2C, with specific support for NDEF Text records carrying JSON payloads. Multiple records per NDEF message are supported.

## Architecture

### HAL (Hardware Abstraction Layer)
The I2C layer is processor-agnostic. The library defines a function pointer interface that the caller populates with their platform's I2C read/write routines before use. No direct register I/O is performed inside the library.

### Layers
1. **I2C HAL** — caller-supplied callbacks; the library never calls platform I2C directly
2. **ST25DVxx driver** — register-level operations (mailbox, EEPROM areas, capability container)
3. **NDEF parser/builder** — TLV framing, record header encoding/decoding, multi-record messages
4. **Text record + JSON** — UTF-8 text record handling; JSON is treated as opaque payload within the text record

## Testing

### Framework
[Unity Test Project](https://github.com/ThrowTheSwitch/Unity) — fetched automatically via CMake `FetchContent`. Tests compile and run natively on the host (not cross-compiled).

```sh
cmake -S . -B build          # configure (fetches Unity on first run)
cmake --build build          # build library, tests, and tools
ctest --test-dir build       # run all tests
ctest --test-dir build --output-on-failure  # show test output on failure
```

### Mock HAL
`test/mock_hal.c` backs I2C reads/writes with a `uint8_t eeprom[8192]` buffer. Pre-populate with known CC + NDEF bytes for read tests; assert buffer contents after write tests.

### Coverage required

**NDEF TLV framing**
- 1-byte length field (payload ≤ 254 bytes)
- 3-byte length field (`0xFF` + 2-byte big-endian, payload ≥ 255 bytes)
- Terminator `0xFE` present and correctly positioned

**NDEF record header**
- SR=1 (short record, 1-byte payload length ≤ 255)
- SR=0 (long record, 4-byte payload length)
- Boundary: exactly 255-byte payload (SR=1 max) and 256-byte payload (forces SR=0)
- MB/ME flags: set on first/last record; neither set on middle records
- IL flag and ID field handling

**Multi-record messages**
- 2 short records — MB/ME flag placement
- 3+ records — middle record has neither MB nor ME
- Mixed short + long records — SR bit toggles correctly mid-message
- Long record first, short record second — TLV length field crosses 0xFF threshold
- Maximum-length record followed by another record — parser does not over-read
- Single-byte payload record (SR=1, length=1) — minimal valid record

**Round-trip**
- Encode N variable-length text records → write to mock EEPROM → read back → decode → assert byte-identical to input
- Covers encode/decode asymmetry that directional unit tests miss

**Text record + JSON**
- UTF-8 language tag encoding/decoding
- JSON payload preserved verbatim (no re-serialization)
- Multi-record message where each record carries a distinct JSON object

### Out of scope
Integration tests against real hardware are the caller's responsibility.

## Datasheets

Register addresses, timing, and capability container values must be verified against the official datasheets:

- **ST25DV04K / ST25DV16K / ST25DV64K** (original series): https://www.st.com/resource/en/datasheet/st25dv04k.pdf
- **ST25DV04KC / ST25DV16KC / ST25DV64KC** (C-variant series): https://www.st.com/resource/en/datasheet/st25dv04kc.pdf

Also relevant: [errata sheet ES0616](https://www.st.com/resource/en/errata_sheet/es0616-st25dv04k-st25dv16k-and-st25dv64k-device-limitations-stmicroelectronics.pdf) covering known device limitations.

### ST25DVxx memory model
- User memory is I2C EEPROM, addressed in 256-byte blocks
- NDEF message starts at address 0x0000 with a TLV wrapper (tag `0x03`, length, value, terminator `0xFE`)
- Capability Container (CC) lives at 0x0000 in the RF address space but at a fixed I2C offset — read it to confirm NDEF support before any operation
