#ifndef NDEF_H
#define NDEF_H

#include <stdint.h>
#include <stdbool.h>

/* Type Name Format values */
#define NDEF_TNF_EMPTY          0x00U
#define NDEF_TNF_WELL_KNOWN     0x01U
#define NDEF_TNF_MIME           0x02U
#define NDEF_TNF_ABSOLUTE_URI   0x03U
#define NDEF_TNF_EXTERNAL       0x04U
#define NDEF_TNF_UNKNOWN        0x05U

/* Well-known type bytes */
#define NDEF_TYPE_TEXT          ((uint8_t)'T')
#define NDEF_TYPE_URI           ((uint8_t)'U')

/* Error codes */
#define NDEF_OK                 0
#define NDEF_ERR_PARAM         -1
#define NDEF_ERR_OVERFLOW      -2
#define NDEF_ERR_MALFORMED     -3
#define NDEF_ERR_NO_RECORDS    -4

/**
 * A single NDEF record.
 *
 * After ndef_parse(), type/id/payload point into the source buffer — the
 * buffer must remain valid for the lifetime of these records. No copies
 * are made during parsing.
 *
 * For ndef_build(), the caller populates all fields before passing the
 * array to the builder.
 */
typedef struct {
    uint8_t        tnf;
    const uint8_t *type;
    uint8_t        type_len;
    const uint8_t *id;
    uint8_t        id_len;
    const uint8_t *payload;
    uint32_t       payload_len;
} ndef_record_t;

/**
 * Parse a bare NDEF message (no TLV wrapper) from buf.
 * Populates up to max_records entries in records[].
 *
 * @return Number of records parsed (≥1), or a negative error code.
 */
int ndef_parse(const uint8_t *buf, uint16_t buf_len,
               ndef_record_t *records, uint8_t max_records);

/**
 * Serialize records[] into buf as a bare NDEF message (no TLV wrapper).
 *
 * @return Bytes written, or a negative error code.
 */
int ndef_build(const ndef_record_t *records, uint8_t num_records,
               uint8_t *buf, uint16_t buf_len);

#endif /* NDEF_H */
