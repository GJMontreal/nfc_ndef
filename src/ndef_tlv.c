#include "ndef_tlv.h"
#include <string.h>

int ndef_tlv_wrap(const uint8_t *ndef_msg, uint16_t ndef_len,
                  uint8_t *buf, uint16_t buf_len)
{
    uint16_t pos = 0;

    if (!ndef_msg || !buf)
        return NDEF_ERR_PARAM;

    if (pos >= buf_len) return NDEF_ERR_OVERFLOW;
    buf[pos++] = NDEF_TLV_NDEF;

    if (ndef_len < NDEF_TLV_LONG_FORM) {
        if (pos >= buf_len) return NDEF_ERR_OVERFLOW;
        buf[pos++] = (uint8_t)ndef_len;
    } else {
        if (pos + 3u > buf_len) return NDEF_ERR_OVERFLOW;
        buf[pos++] = NDEF_TLV_LONG_FORM;
        buf[pos++] = (uint8_t)(ndef_len >> 8);
        buf[pos++] = (uint8_t)(ndef_len);
    }

    if (pos + ndef_len > buf_len) return NDEF_ERR_OVERFLOW;
    memcpy(&buf[pos], ndef_msg, ndef_len);
    pos += ndef_len;

    if (pos >= buf_len) return NDEF_ERR_OVERFLOW;
    buf[pos++] = NDEF_TLV_TERMINATOR;

    return (int)pos;
}

int ndef_tlv_unwrap(const uint8_t *buf, uint16_t buf_len,
                    const uint8_t **ndef_msg, uint16_t *ndef_len)
{
    uint16_t pos = 0;

    if (!buf || !ndef_msg || !ndef_len)
        return NDEF_ERR_PARAM;

    while (pos < buf_len) {
        uint8_t  tag;
        uint16_t len;

        tag = buf[pos++];

        if (tag == NDEF_TLV_TERMINATOR)
            break;

        if (tag == NDEF_TLV_NULL)
            continue;

        if (pos >= buf_len) return NDEF_ERR_MALFORMED;

        if (buf[pos] == NDEF_TLV_LONG_FORM) {
            pos++;
            if (pos + 2u > buf_len) return NDEF_ERR_MALFORMED;
            len  = (uint16_t)buf[pos]     << 8;
            len |= (uint16_t)buf[pos + 1];
            pos += 2;
        } else {
            len = buf[pos++];
        }

        if (tag == NDEF_TLV_NDEF) {
            if (pos + len > buf_len) return NDEF_ERR_MALFORMED;
            *ndef_msg = &buf[pos];
            *ndef_len = len;
            return NDEF_OK;
        }

        /* Skip unknown TLV value */
        pos += len;
    }

    return NDEF_ERR_NO_RECORDS;
}
