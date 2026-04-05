#ifndef NDEF_TLV_H
#define NDEF_TLV_H

#include <stdint.h>
#include "ndef.h"

/* TLV tags used in EEPROM framing */
#define NDEF_TLV_NULL           0x00U
#define NDEF_TLV_NDEF           0x03U
#define NDEF_TLV_TERMINATOR     0xFEU
#define NDEF_TLV_LONG_FORM      0xFFU   /* next 2 bytes carry the length */

/**
 * Wrap a bare NDEF message in a TLV envelope for writing to EEPROM.
 * Uses a 1-byte length field for messages ≤254 bytes, 3-byte form otherwise.
 *
 * @return Bytes written (tag + length + value + terminator), or negative error.
 */
int ndef_tlv_wrap(const uint8_t *ndef_msg, uint16_t ndef_len,
                  uint8_t *buf, uint16_t buf_len);

/**
 * Locate the NDEF TLV in buf and return a pointer + length to the bare
 * NDEF message inside it. No copy is made; buf must remain valid.
 *
 * @return NDEF_OK or a negative error code.
 */
int ndef_tlv_unwrap(const uint8_t *buf, uint16_t buf_len,
                    const uint8_t **ndef_msg, uint16_t *ndef_len);

#endif /* NDEF_TLV_H */
