#include "ndef.h"
#include <string.h>

/* Record header flag masks */
#define HDR_MB      0x80U
#define HDR_ME      0x40U
#define HDR_CF      0x20U
#define HDR_SR      0x10U
#define HDR_IL      0x08U
#define HDR_TNF     0x07U

int ndef_parse(const uint8_t *buf, uint16_t buf_len,
               ndef_record_t *records, uint8_t max_records)
{
    uint16_t pos   = 0;
    uint8_t  count = 0;
    bool     first = true;

    if (!buf || !records || max_records == 0)
        return NDEF_ERR_PARAM;

    while (pos < buf_len && count < max_records) {
        ndef_record_t *rec = &records[count];
        uint8_t  hdr;
        uint8_t  type_len;
        uint32_t payload_len;
        uint8_t  id_len = 0;

        hdr = buf[pos++];

        if (first && !(hdr & HDR_MB))
            return NDEF_ERR_MALFORMED;
        first = false;

        rec->tnf = hdr & HDR_TNF;

        if (pos >= buf_len) return NDEF_ERR_MALFORMED;
        type_len = buf[pos++];

        if (hdr & HDR_SR) {
            if (pos >= buf_len) return NDEF_ERR_MALFORMED;
            payload_len = buf[pos++];
        } else {
            if (pos + 4u > buf_len) return NDEF_ERR_MALFORMED;
            payload_len  = (uint32_t)buf[pos]     << 24;
            payload_len |= (uint32_t)buf[pos + 1] << 16;
            payload_len |= (uint32_t)buf[pos + 2] << 8;
            payload_len |= (uint32_t)buf[pos + 3];
            pos += 4;
        }

        if (hdr & HDR_IL) {
            if (pos >= buf_len) return NDEF_ERR_MALFORMED;
            id_len = buf[pos++];
        }

        if (pos + type_len > buf_len) return NDEF_ERR_MALFORMED;
        rec->type     = &buf[pos];
        rec->type_len = type_len;
        pos += type_len;

        if (id_len > 0) {
            if (pos + id_len > buf_len) return NDEF_ERR_MALFORMED;
            rec->id     = &buf[pos];
            rec->id_len = id_len;
            pos += id_len;
        } else {
            rec->id     = NULL;
            rec->id_len = 0;
        }

        /* payload_len bounds-check: cast is safe after check */
        if (payload_len > (uint32_t)(buf_len - pos)) return NDEF_ERR_MALFORMED;
        rec->payload     = &buf[pos];
        rec->payload_len = payload_len;
        pos += (uint16_t)payload_len;

        count++;

        if (hdr & HDR_ME)
            break;
    }

    return (count > 0) ? (int)count : NDEF_ERR_NO_RECORDS;
}

int ndef_build(const ndef_record_t *records, uint8_t num_records,
               uint8_t *buf, uint16_t buf_len)
{
    uint16_t pos = 0;

    if (!records || num_records == 0 || !buf || buf_len == 0)
        return NDEF_ERR_PARAM;

    for (uint8_t i = 0; i < num_records; i++) {
        const ndef_record_t *rec = &records[i];
        bool    sr  = (rec->payload_len <= 0xFFU);
        uint8_t hdr = rec->tnf & HDR_TNF;

        if (i == 0)               hdr |= HDR_MB;
        if (i == num_records - 1) hdr |= HDR_ME;
        if (sr)                   hdr |= HDR_SR;
        if (rec->id_len > 0)      hdr |= HDR_IL;

        if (pos >= buf_len) return NDEF_ERR_OVERFLOW;
        buf[pos++] = hdr;

        if (pos >= buf_len) return NDEF_ERR_OVERFLOW;
        buf[pos++] = rec->type_len;

        if (sr) {
            if (pos >= buf_len) return NDEF_ERR_OVERFLOW;
            buf[pos++] = (uint8_t)rec->payload_len;
        } else {
            if (pos + 4u > buf_len) return NDEF_ERR_OVERFLOW;
            buf[pos++] = (uint8_t)(rec->payload_len >> 24);
            buf[pos++] = (uint8_t)(rec->payload_len >> 16);
            buf[pos++] = (uint8_t)(rec->payload_len >>  8);
            buf[pos++] = (uint8_t)(rec->payload_len);
        }

        if (rec->id_len > 0) {
            if (pos >= buf_len) return NDEF_ERR_OVERFLOW;
            buf[pos++] = rec->id_len;
        }

        if (rec->type_len > 0) {
            if (pos + rec->type_len > buf_len) return NDEF_ERR_OVERFLOW;
            memcpy(&buf[pos], rec->type, rec->type_len);
            pos += rec->type_len;
        }

        if (rec->id_len > 0) {
            if (pos + rec->id_len > buf_len) return NDEF_ERR_OVERFLOW;
            memcpy(&buf[pos], rec->id, rec->id_len);
            pos += rec->id_len;
        }

        if (rec->payload_len > 0) {
            if (rec->payload_len > (uint32_t)(buf_len - pos))
                return NDEF_ERR_OVERFLOW;
            memcpy(&buf[pos], rec->payload, rec->payload_len);
            pos += (uint16_t)rec->payload_len;
        }
    }

    return (int)pos;
}

