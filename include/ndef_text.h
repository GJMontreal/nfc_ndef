#ifndef NDEF_TEXT_H
#define NDEF_TEXT_H

#include <stdint.h>
#include "ndef.h"

/* Maximum IANA language code length (e.g. "en", "fr", "zh-CN") */
#define NDEF_TEXT_LANG_MAX      35U

/* Status byte encoding (bit 7) */
#define NDEF_TEXT_ENC_UTF8      0x00U
#define NDEF_TEXT_ENC_UTF16     0x80U

/**
 * Encode a UTF-8 text record payload into buf.
 *
 * @param lang      Null-terminated IANA language code (e.g. "en").
 * @param text      Null-terminated UTF-8 string (plain text or JSON).
 * @param buf       Destination buffer for the encoded payload.
 * @param buf_len   Size of buf in bytes.
 * @return          Bytes written to buf, or a negative error code.
 */
int ndef_text_encode(const char *lang, const char *text,
                     uint8_t *buf, uint16_t buf_len);

/**
 * Decode a text record payload.
 *
 * @param payload       Raw payload bytes from an ndef_record_t.
 * @param payload_len   Length of payload in bytes.
 * @param lang_buf      Receives null-terminated language code.
 *                      Must be at least NDEF_TEXT_LANG_MAX + 1 bytes.
 * @param lang_buf_len  Size of lang_buf.
 * @param text_buf      Receives null-terminated text (or JSON).
 * @param text_buf_len  Size of text_buf.
 * @return              NDEF_OK or a negative error code.
 */
int ndef_text_decode(const uint8_t *payload, uint32_t payload_len,
                     char *lang_buf, uint8_t lang_buf_len,
                     char *text_buf, uint16_t text_buf_len);

/**
 * Convenience: encode text into payload_buf and populate record with
 * TNF=WELL_KNOWN, type="T", and the encoded payload.
 *
 * payload_buf must remain valid for the lifetime of the record.
 *
 * @return Bytes written to payload_buf, or a negative error code.
 */
int ndef_text_record(const char *lang, const char *text,
                     uint8_t *payload_buf, uint16_t payload_buf_len,
                     ndef_record_t *record);

#endif /* NDEF_TEXT_H */
