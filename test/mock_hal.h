#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include "st25dv_hal.h"

#define MOCK_EEPROM_SIZE    8192U

/*
 * Default CC pre-populated by mock_hal_init():
 *   [0] = 0xE2  magic
 *   [1] = 0x40  version 1.0, read/write
 *   [2] = 0x3F  MLEN → (0x3F+1)*8 = 512 bytes
 *   [3] = 0x00  access conditions
 * NDEF data starts at offset 0x0004.
 */
typedef struct {
    uint8_t eeprom[MOCK_EEPROM_SIZE];
} mock_hal_ctx_t;

/* Zero-fill EEPROM and write the default CC. */
void mock_hal_init(mock_hal_ctx_t *ctx);

/* Return a st25dv_hal_t backed by ctx. */
st25dv_hal_t mock_hal_get(mock_hal_ctx_t *ctx);

#endif /* MOCK_HAL_H */
