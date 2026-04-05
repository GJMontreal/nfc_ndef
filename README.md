# nfc_ndef

Minimal embedded C library for ST25DVxx NFC tags. Reads and writes NDEF messages over I2C, with specific support for NDEF Text records carrying JSON payloads. Multiple records per message are supported.

No dynamic memory allocation. All buffers are caller-supplied.

## Supported hardware

ST25DV04K, ST25DV16K, ST25DV64K and their C-variants (ST25DV04KC, ST25DV16KC, ST25DV64KC). Standard 4-byte Capability Container only; the ST25DV64K Extended CC is not currently supported.

## Integration

Add `include/` to your include path and compile `src/st25dv.c`, `src/ndef.c`, and `src/ndef_text.c` alongside your application. No build system is required.

Implement the two I2C callbacks for your platform and populate a `st25dv_hal_t`:

```c
#include "st25dv_hal.h"
#include "st25dv.h"

int my_i2c_read(uint8_t dev_addr, uint16_t mem_addr,
                uint8_t *buf, uint16_t len, void *ctx)
{
    /* platform I2C read — return 0 on success */
}

int my_i2c_write(uint8_t dev_addr, uint16_t mem_addr,
                 const uint8_t *buf, uint16_t len, void *ctx)
{
    /* platform I2C write — return 0 on success */
}

st25dv_hal_t hal = { .read = my_i2c_read, .write = my_i2c_write, .ctx = NULL };
```

## Usage

### Initialise

```c
st25dv_t dev;
int rc = st25dv_init(&dev, &hal);
/* Reads the Capability Container to confirm NDEF support.
   Returns ST25DV_OK or a negative error code. */
```

### Write a JSON payload

```c
#include "ndef.h"
#include "ndef_text.h"

uint8_t payload_buf[64];
ndef_record_t rec;
ndef_text_record("en", "{\"temp\":23.5}", payload_buf, sizeof(payload_buf), &rec);

uint8_t ndef_buf[128], tlv_buf[136];
int ndef_len = ndef_build(&rec, 1, ndef_buf, sizeof(ndef_buf));
int tlv_len  = ndef_tlv_wrap(ndef_buf, ndef_len, tlv_buf, sizeof(tlv_buf));

st25dv_ndef_write(&dev, tlv_buf, tlv_len);
```

### Read and parse

```c
uint8_t raw[512];
uint16_t raw_len;
st25dv_ndef_read(&dev, raw, sizeof(raw), &raw_len);

const uint8_t *ndef_msg;
uint16_t       ndef_len;
ndef_tlv_unwrap(raw, raw_len, &ndef_msg, &ndef_len);

ndef_record_t records[8];
int n = ndef_parse(ndef_msg, ndef_len, records, 8);

for (int i = 0; i < n; i++) {
    char lang[8], text[256];
    ndef_text_decode(records[i].payload, records[i].payload_len,
                     lang, sizeof(lang), text, sizeof(text));
    /* text contains the JSON string */
}
```

## dump_parse

`tools/dump_parse.c` is a host-side utility that reads a raw EEPROM dump (binary file) and prints the Capability Container details and all parsed NDEF records to stdout. Useful for inspecting dumps captured from a real device.

```sh
./build/dump_parse <file.bin>
```

An example dump from a real ST25DV16K is included:

```sh
./build/dump_parse examples/st25dv_single_text_record.bin
```

```
CC: magic=0xE2  ver=0x40  MLEN=0xFF (2048 bytes)  access=0x01

NDEF message: 26 bytes

1 record(s) found:

  [0] TNF=0x1  type="T" (1 byte)  payload=22 bytes
       lang="en"  text={"pattern":"chase"}
```

The tool is built automatically alongside the tests when you run `cmake --build build`.

## Building and running tests

Tests run natively on the host. The [Unity Test Project](https://github.com/ThrowTheSwitch/Unity) is fetched automatically by CMake on first configure.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Error codes

All functions return `0` (`*_OK`) on success or a negative value on error.

| Code | Meaning |
|---|---|
| `ST25DV_ERR_HAL` | I2C callback returned non-zero |
| `ST25DV_ERR_NO_NDEF` | Capability Container magic byte not found |
| `ST25DV_ERR_PARAM` | NULL pointer or invalid argument |
| `ST25DV_ERR_OVERFLOW` | Buffer too small or write exceeds memory |
| `NDEF_ERR_MALFORMED` | Record or TLV structure is invalid |
| `NDEF_ERR_NO_RECORDS` | No NDEF TLV found in buffer |
| `NDEF_ERR_OVERFLOW` | Output buffer too small |

## Datasheets

- [ST25DV04K / ST25DV16K / ST25DV64K](https://www.st.com/resource/en/datasheet/st25dv04k.pdf)
- [ST25DV04KC / ST25DV16KC / ST25DV64KC](https://www.st.com/resource/en/datasheet/st25dv04kc.pdf)
