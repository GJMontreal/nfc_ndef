#include "unity.h"
#include "ndef_tlv.h"
#include "ndef.h"
#include "test_helpers.h"
#include <string.h>

void setUp(void)    {}
void tearDown(void) {}

/* ---------------------------------------------------------- TLV wrap/unwrap */

void test_tlv_wrap_short_length(void)
{
    /* NDEF message ≤ 254 bytes → 1-byte TLV length field */
    uint8_t msg[4]  = { 0x01, 0x02, 0x03, 0x04 };
    uint8_t out[16] = { 0 };

    int rc = ndef_tlv_wrap(msg, sizeof(msg), out, sizeof(out));

    TEST_ASSERT_EQUAL(7, rc);        /* 1 tag + 1 len + 4 value + 1 term */
    TEST_ASSERT_EQUAL_HEX8(NDEF_TLV_NDEF,        out[0]);
    TEST_ASSERT_EQUAL_HEX8(4,                    out[1]);
    TEST_ASSERT_EQUAL_HEX8(NDEF_TLV_TERMINATOR,  out[6]);
}

void test_tlv_wrap_long_length(void)
{
    /* NDEF message ≥ 255 bytes → 3-byte TLV length field (0xFF + 2 bytes) */
    uint8_t msg[255];
    uint8_t out[264];
    memset(msg, 0xAA, sizeof(msg));

    int rc = ndef_tlv_wrap(msg, 255, out, sizeof(out));

    TEST_ASSERT_EQUAL(260, rc);      /* 1 tag + 3 len + 255 value + 1 term */
    TEST_ASSERT_EQUAL_HEX8(NDEF_TLV_NDEF,       out[0]);
    TEST_ASSERT_EQUAL_HEX8(NDEF_TLV_LONG_FORM,  out[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00,                out[2]);
    TEST_ASSERT_EQUAL_HEX8(0xFF,                out[3]);
    TEST_ASSERT_EQUAL_HEX8(NDEF_TLV_TERMINATOR, out[259]);
}

void test_tlv_unwrap_short_length(void)
{
    uint8_t buf[]           = { 0x03, 0x03, 0xAA, 0xBB, 0xCC, 0xFE };
    const uint8_t *ndef_msg = NULL;
    uint16_t ndef_len       = 0;

    int rc = ndef_tlv_unwrap(buf, sizeof(buf), &ndef_msg, &ndef_len);

    TEST_ASSERT_EQUAL(NDEF_OK, rc);
    TEST_ASSERT_EQUAL(3, ndef_len);
    TEST_ASSERT_EQUAL_PTR(&buf[2], ndef_msg);
}

void test_tlv_unwrap_long_length(void)
{
    uint8_t buf[270];
    memset(buf, 0xBB, sizeof(buf));
    buf[0] = NDEF_TLV_NDEF;
    buf[1] = NDEF_TLV_LONG_FORM;
    buf[2] = 0x01;
    buf[3] = 0x00;  /* length = 0x0100 = 256 */
    buf[4 + 256] = NDEF_TLV_TERMINATOR;

    const uint8_t *ndef_msg = NULL;
    uint16_t       ndef_len = 0;

    int rc = ndef_tlv_unwrap(buf, sizeof(buf), &ndef_msg, &ndef_len);

    TEST_ASSERT_EQUAL(NDEF_OK, rc);
    TEST_ASSERT_EQUAL(256, ndef_len);
    TEST_ASSERT_EQUAL_PTR(&buf[4], ndef_msg);
}

void test_tlv_roundtrip(void)
{
    uint8_t msg[100];
    uint8_t wrapped[200];
    memset(msg, 0x5A, sizeof(msg));

    int wrapped_len = ndef_tlv_wrap(msg, sizeof(msg), wrapped, sizeof(wrapped));
    TEST_ASSERT_GREATER_THAN(0, wrapped_len);

    const uint8_t *out     = NULL;
    uint16_t       out_len = 0;
    TEST_ASSERT_EQUAL(NDEF_OK, ndef_tlv_unwrap(wrapped, (uint16_t)wrapped_len,
                                               &out, &out_len));
    TEST_ASSERT_EQUAL(sizeof(msg), out_len);
    TEST_ASSERT_EQUAL_MEMORY(msg, out, sizeof(msg));
}

/* ----------------------------------------- combined TLV + NDEF round-trip */

void test_tlv_then_ndef_roundtrip(void)
{
    static const uint8_t type_t = 'T';
    static const uint8_t pay[]  = { 0x02, 'e', 'n', '{', '"', 'k', '"', ':', '1', '}' };

    ndef_record_t rec;
    uint8_t ndef_buf[64], tlv_buf[72];

    make_record(&rec, NDEF_TNF_WELL_KNOWN, &type_t, 1, pay, sizeof(pay));

    int ndef_len = ndef_build(&rec, 1, ndef_buf, sizeof(ndef_buf));
    TEST_ASSERT_GREATER_THAN(0, ndef_len);

    int tlv_len = ndef_tlv_wrap(ndef_buf, (uint16_t)ndef_len, tlv_buf, sizeof(tlv_buf));
    TEST_ASSERT_GREATER_THAN(0, tlv_len);

    const uint8_t *unwrapped     = NULL;
    uint16_t       unwrapped_len = 0;
    TEST_ASSERT_EQUAL(NDEF_OK, ndef_tlv_unwrap(tlv_buf, (uint16_t)tlv_len,
                                               &unwrapped, &unwrapped_len));

    ndef_record_t parsed;
    TEST_ASSERT_EQUAL(1, ndef_parse(unwrapped, unwrapped_len, &parsed, 1));
    TEST_ASSERT_EQUAL(sizeof(pay), parsed.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(pay, parsed.payload, sizeof(pay));
}

/* --------------------------------------------------------- malformed / error paths */

void test_tlv_unwrap_null_buf(void)
{
    const uint8_t *msg = NULL;
    uint16_t       len = 0;
    TEST_ASSERT_EQUAL(NDEF_ERR_PARAM, ndef_tlv_unwrap(NULL, 10, &msg, &len));
}

void test_tlv_unwrap_only_terminator(void)
{
    /* A buffer containing only the terminator — no NDEF TLV present */
    static const uint8_t buf[] = { NDEF_TLV_TERMINATOR };
    const uint8_t *msg = NULL;
    uint16_t       len = 0;
    TEST_ASSERT_EQUAL(NDEF_ERR_NO_RECORDS, ndef_tlv_unwrap(buf, sizeof(buf), &msg, &len));
}

void test_tlv_unwrap_truncated_value(void)
{
    /* Length byte says 10 bytes follow, but buffer ends after 3 */
    static const uint8_t buf[] = { NDEF_TLV_NDEF, 0x0A, 0x01, 0x02, 0x03 };
    const uint8_t *msg = NULL;
    uint16_t       len = 0;
    TEST_ASSERT_EQUAL(NDEF_ERR_MALFORMED, ndef_tlv_unwrap(buf, sizeof(buf), &msg, &len));
}

void test_tlv_unwrap_long_form_truncated_length(void)
{
    /* 0xFF marker present but only 1 byte of the 2-byte length follows */
    static const uint8_t buf[] = { NDEF_TLV_NDEF, NDEF_TLV_LONG_FORM, 0x01 };
    const uint8_t *msg = NULL;
    uint16_t       len = 0;
    TEST_ASSERT_EQUAL(NDEF_ERR_MALFORMED, ndef_tlv_unwrap(buf, sizeof(buf), &msg, &len));
}

void test_tlv_wrap_buffer_too_small(void)
{
    static const uint8_t msg[] = { 0x01, 0x02, 0x03 };
    uint8_t out[2];  /* too small for tag + length + value + terminator */
    TEST_ASSERT_EQUAL(NDEF_ERR_OVERFLOW, ndef_tlv_wrap(msg, sizeof(msg), out, sizeof(out)));
}

void test_tlv_wrap_null_msg(void)
{
    uint8_t out[16];
    TEST_ASSERT_EQUAL(NDEF_ERR_PARAM, ndef_tlv_wrap(NULL, 4, out, sizeof(out)));
}

/* --------------------------------------------------------------------- main */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_tlv_wrap_short_length);
    RUN_TEST(test_tlv_wrap_long_length);
    RUN_TEST(test_tlv_unwrap_short_length);
    RUN_TEST(test_tlv_unwrap_long_length);
    RUN_TEST(test_tlv_roundtrip);
    RUN_TEST(test_tlv_then_ndef_roundtrip);

    RUN_TEST(test_tlv_unwrap_null_buf);
    RUN_TEST(test_tlv_unwrap_only_terminator);
    RUN_TEST(test_tlv_unwrap_truncated_value);
    RUN_TEST(test_tlv_unwrap_long_form_truncated_length);
    RUN_TEST(test_tlv_wrap_buffer_too_small);
    RUN_TEST(test_tlv_wrap_null_msg);

    return UNITY_END();
}
