#include "ndef_text.h"
#include <string.h>

static const uint8_t k_type_text = NDEF_TYPE_TEXT;

int ndef_text_encode(const char *lang, const char *text,
                     uint8_t *buf, uint16_t buf_len)
{
    uint8_t  lang_len;
    uint16_t text_len;
    uint16_t pos = 0;

    if (!lang || !text || !buf || buf_len == 0)
        return NDEF_ERR_PARAM;

    lang_len = (uint8_t)strlen(lang);
    text_len = (uint16_t)strlen(text);

    if (lang_len > NDEF_TEXT_LANG_MAX)
        return NDEF_ERR_PARAM;

    if ((uint32_t)1u + lang_len + text_len > buf_len)
        return NDEF_ERR_OVERFLOW;

    buf[pos++] = NDEF_TEXT_ENC_UTF8 | (lang_len & 0x3Fu);
    memcpy(&buf[pos], lang, lang_len);
    pos += lang_len;
    memcpy(&buf[pos], text, text_len);
    pos += text_len;

    return (int)pos;
}

int ndef_text_decode(const uint8_t *payload, uint32_t payload_len,
                     char *lang_buf, uint8_t lang_buf_len,
                     char *text_buf, uint16_t text_buf_len)
{
    uint8_t  lang_len;
    uint32_t text_len;

    if (!payload || payload_len < 1u || !lang_buf || !text_buf)
        return NDEF_ERR_PARAM;

    lang_len = payload[0] & 0x3Fu;

    if ((uint32_t)(1u + lang_len) > payload_len)
        return NDEF_ERR_MALFORMED;

    text_len = payload_len - 1u - lang_len;

    if ((uint8_t)(lang_len + 1u) > lang_buf_len)
        return NDEF_ERR_OVERFLOW;
    if (text_len + 1u > text_buf_len)
        return NDEF_ERR_OVERFLOW;

    memcpy(lang_buf, &payload[1], lang_len);
    lang_buf[lang_len] = '\0';

    memcpy(text_buf, &payload[1u + lang_len], text_len);
    text_buf[text_len] = '\0';

    return NDEF_OK;
}

int ndef_text_record(const char *lang, const char *text,
                     uint8_t *payload_buf, uint16_t payload_buf_len,
                     ndef_record_t *record)
{
    int rc;

    if (!record)
        return NDEF_ERR_PARAM;

    rc = ndef_text_encode(lang, text, payload_buf, payload_buf_len);
    if (rc < 0)
        return rc;

    record->tnf         = NDEF_TNF_WELL_KNOWN;
    record->type        = &k_type_text;
    record->type_len    = 1u;
    record->id          = NULL;
    record->id_len      = 0u;
    record->payload     = payload_buf;
    record->payload_len = (uint32_t)rc;

    return rc;
}
