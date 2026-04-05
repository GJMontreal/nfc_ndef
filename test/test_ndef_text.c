#include "unity.h"
#include "ndef_text.h"
#include "ndef.h"
#include "ndef_tlv.h"
#include <string.h>

void setUp(void)    {}
void tearDown(void) {}

/* -------------------------------------------------- encode / decode basics */

void test_encode_simple(void)
{
    uint8_t buf[32];
    int rc = ndef_text_encode("en", "hello", buf, sizeof(buf));

    /* status(1) + "en"(2) + "hello"(5) = 8 */
    TEST_ASSERT_EQUAL(8, rc);
    TEST_ASSERT_EQUAL_HEX8(0x02, buf[0]);  /* UTF-8, lang_len=2 */
    TEST_ASSERT_EQUAL_HEX8('e',  buf[1]);
    TEST_ASSERT_EQUAL_HEX8('n',  buf[2]);
    TEST_ASSERT_EQUAL_HEX8('h',  buf[3]);
}

void test_decode_simple(void)
{
    uint8_t buf[32];
    int enc = ndef_text_encode("en", "hello", buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, enc);

    char lang[8], text[32];
    int rc = ndef_text_decode(buf, (uint32_t)enc,
                              lang, sizeof(lang),
                              text, sizeof(text));

    TEST_ASSERT_EQUAL(NDEF_OK, rc);
    TEST_ASSERT_EQUAL_STRING("en",    lang);
    TEST_ASSERT_EQUAL_STRING("hello", text);
}

void test_encode_decode_json(void)
{
    const char *json = "{\"temp\":23.5,\"unit\":\"C\"}";
    uint8_t buf[64];
    char    lang[8], text[64];

    int enc = ndef_text_encode("en", json, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, enc);

    TEST_ASSERT_EQUAL(NDEF_OK,
        ndef_text_decode(buf, (uint32_t)enc,
                         lang, sizeof(lang),
                         text, sizeof(text)));

    TEST_ASSERT_EQUAL_STRING("en",   lang);
    TEST_ASSERT_EQUAL_STRING(json,   text);
}

void test_encode_language_code_preserved(void)
{
    uint8_t buf[64];
    char    lang[16], text[32];

    int enc = ndef_text_encode("zh-CN", "test", buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, enc);

    TEST_ASSERT_EQUAL(NDEF_OK,
        ndef_text_decode(buf, (uint32_t)enc,
                         lang, sizeof(lang),
                         text, sizeof(text)));
    TEST_ASSERT_EQUAL_STRING("zh-CN", lang);
}

/* ---------------------------------------- ndef_text_record helper */

void test_text_record_helper(void)
{
    uint8_t       payload_buf[64];
    ndef_record_t rec;

    int rc = ndef_text_record("en", "{\"x\":1}", payload_buf, sizeof(payload_buf), &rec);

    TEST_ASSERT_GREATER_THAN(0, rc);
    TEST_ASSERT_EQUAL_HEX8(NDEF_TNF_WELL_KNOWN, rec.tnf);
    TEST_ASSERT_EQUAL(1, rec.type_len);
    TEST_ASSERT_EQUAL_HEX8(NDEF_TYPE_TEXT, rec.type[0]);
    TEST_ASSERT_EQUAL_PTR(payload_buf, rec.payload);
    TEST_ASSERT_EQUAL((uint32_t)rc, rec.payload_len);
}

/* ---------------------------------------- multi-record text message */

void test_multi_record_text_json(void)
{
    const char *jsons[] = {
        "{\"sensor\":\"temp\",\"value\":21}",
        "{\"sensor\":\"hum\",\"value\":55}",
        "{\"sensor\":\"co2\",\"value\":412}",
    };

    uint8_t       payloads[3][128];
    ndef_record_t recs[3];

    for (int i = 0; i < 3; i++) {
        int rc = ndef_text_record("en", jsons[i],
                                  payloads[i], sizeof(payloads[i]),
                                  &recs[i]);
        TEST_ASSERT_GREATER_THAN(0, rc);
    }

    uint8_t built[512];
    int built_len = ndef_build(recs, 3, built, sizeof(built));
    TEST_ASSERT_GREATER_THAN(0, built_len);

    ndef_record_t parsed[3];
    TEST_ASSERT_EQUAL(3, ndef_parse(built, (uint16_t)built_len, parsed, 3));

    for (int i = 0; i < 3; i++) {
        char lang[8], text[128];
        TEST_ASSERT_EQUAL(NDEF_OK,
            ndef_text_decode(parsed[i].payload, parsed[i].payload_len,
                             lang, sizeof(lang),
                             text, sizeof(text)));
        TEST_ASSERT_EQUAL_STRING("en",    lang);
        TEST_ASSERT_EQUAL_STRING(jsons[i], text);
    }
}

void test_multi_record_json_tlv_roundtrip(void)
{
    const char *jsons[] = {
        "{\"a\":1}",
        "{\"b\":\"hello world\"}",
    };

    uint8_t       payloads[2][64];
    ndef_record_t recs[2];

    for (int i = 0; i < 2; i++)
        ndef_text_record("en", jsons[i], payloads[i], sizeof(payloads[i]), &recs[i]);

    uint8_t ndef_buf[256], tlv_buf[264];
    int ndef_len = ndef_build(recs, 2, ndef_buf, sizeof(ndef_buf));
    TEST_ASSERT_GREATER_THAN(0, ndef_len);

    int tlv_len = ndef_tlv_wrap(ndef_buf, (uint16_t)ndef_len, tlv_buf, sizeof(tlv_buf));
    TEST_ASSERT_GREATER_THAN(0, tlv_len);

    const uint8_t *unwrapped = NULL;
    uint16_t       unwrapped_len = 0;
    TEST_ASSERT_EQUAL(NDEF_OK,
        ndef_tlv_unwrap(tlv_buf, (uint16_t)tlv_len, &unwrapped, &unwrapped_len));

    ndef_record_t parsed[2];
    TEST_ASSERT_EQUAL(2, ndef_parse(unwrapped, unwrapped_len, parsed, 2));

    for (int i = 0; i < 2; i++) {
        char lang[8], text[64];
        TEST_ASSERT_EQUAL(NDEF_OK,
            ndef_text_decode(parsed[i].payload, parsed[i].payload_len,
                             lang, sizeof(lang), text, sizeof(text)));
        TEST_ASSERT_EQUAL_STRING(jsons[i], text);
    }
}

/* --------------------------------------------------------- malformed / error paths */

void test_decode_empty_payload(void)
{
    static const uint8_t payload[] = { 0x00 };
    char lang[8], text[32];
    /* payload_len=0 → NDEF_ERR_PARAM */
    TEST_ASSERT_EQUAL(NDEF_ERR_PARAM,
        ndef_text_decode(payload, 0, lang, sizeof(lang), text, sizeof(text)));
}

void test_decode_lang_overruns_payload(void)
{
    /* Status byte claims lang_len=5 but only 3 bytes total in payload */
    static const uint8_t payload[] = { 0x05, 'e', 'n' };
    char lang[8], text[32];
    TEST_ASSERT_EQUAL(NDEF_ERR_MALFORMED,
        ndef_text_decode(payload, sizeof(payload), lang, sizeof(lang), text, sizeof(text)));
}

void test_decode_lang_buf_too_small(void)
{
    uint8_t payload[32];
    int enc = ndef_text_encode("en", "hello", payload, sizeof(payload));
    TEST_ASSERT_GREATER_THAN(0, enc);

    char lang[1];   /* too small for "en\0" */
    char text[32];
    TEST_ASSERT_EQUAL(NDEF_ERR_OVERFLOW,
        ndef_text_decode(payload, (uint32_t)enc, lang, sizeof(lang), text, sizeof(text)));
}

void test_decode_text_buf_too_small(void)
{
    uint8_t payload[32];
    int enc = ndef_text_encode("en", "hello world", payload, sizeof(payload));
    TEST_ASSERT_GREATER_THAN(0, enc);

    char lang[8];
    char text[4];   /* too small for "hello world\0" */
    TEST_ASSERT_EQUAL(NDEF_ERR_OVERFLOW,
        ndef_text_decode(payload, (uint32_t)enc, lang, sizeof(lang), text, sizeof(text)));
}

void test_encode_lang_too_long(void)
{
    /* lang longer than NDEF_TEXT_LANG_MAX */
    char lang[NDEF_TEXT_LANG_MAX + 2];
    memset(lang, 'x', sizeof(lang) - 1);
    lang[sizeof(lang) - 1] = '\0';

    uint8_t buf[64];
    TEST_ASSERT_EQUAL(NDEF_ERR_PARAM,
        ndef_text_encode(lang, "text", buf, sizeof(buf)));
}

void test_encode_buffer_too_small(void)
{
    uint8_t buf[4];  /* too small for status + "en" + "hello" */
    TEST_ASSERT_EQUAL(NDEF_ERR_OVERFLOW,
        ndef_text_encode("en", "hello", buf, sizeof(buf)));
}

/* --------------------------------------------------------------------- main */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_encode_simple);
    RUN_TEST(test_decode_simple);
    RUN_TEST(test_encode_decode_json);
    RUN_TEST(test_encode_language_code_preserved);
    RUN_TEST(test_text_record_helper);
    RUN_TEST(test_multi_record_text_json);
    RUN_TEST(test_multi_record_json_tlv_roundtrip);

    RUN_TEST(test_decode_empty_payload);
    RUN_TEST(test_decode_lang_overruns_payload);
    RUN_TEST(test_decode_lang_buf_too_small);
    RUN_TEST(test_decode_text_buf_too_small);
    RUN_TEST(test_encode_lang_too_long);
    RUN_TEST(test_encode_buffer_too_small);

    return UNITY_END();
}
