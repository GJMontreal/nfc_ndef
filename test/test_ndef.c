#include "unity.h"
#include "ndef.h"
#include "test_helpers.h"

#include <string.h>

void setUp(void)    {}
void tearDown(void) {}

/* -------------------------------------------------- single record parse/build */

void test_parse_single_short_record(void)
{
    /* Hand-crafted: MB|ME|SR, TNF=Well-Known, type="T", payload="hi" */
    static const uint8_t buf[] = {
        0xD1,               /* MB | ME | SR | TNF=0x01 */
        0x01,               /* type length = 1 */
        0x02,               /* payload length = 2 (SR) */
        'T',                /* type */
        'h', 'i'            /* payload */
    };
    ndef_record_t rec;

    int rc = ndef_parse(buf, sizeof(buf), &rec, 1);

    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_EQUAL_HEX8(NDEF_TNF_WELL_KNOWN, rec.tnf);
    TEST_ASSERT_EQUAL(1, rec.type_len);
    TEST_ASSERT_EQUAL_HEX8('T', rec.type[0]);
    TEST_ASSERT_EQUAL(2, rec.payload_len);
    TEST_ASSERT_EQUAL_MEMORY("hi", rec.payload, 2);
}

void test_parse_single_long_record(void)
{
    /* SR=0: payload length encoded in 4 bytes */
    uint8_t buf[14];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0xC1;  /* MB | ME | TNF=0x01 (no SR) */
    buf[1] = 0x01;  /* type length */
    buf[2] = 0x00; buf[3] = 0x00; buf[4] = 0x00; buf[5] = 0x05; /* payload_len=5 */
    buf[6] = 'T';
    memcpy(&buf[7], "hello", 5);

    ndef_record_t rec;
    int rc = ndef_parse(buf, sizeof(buf), &rec, 1);

    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_EQUAL(5u, rec.payload_len);
    TEST_ASSERT_EQUAL_MEMORY("hello", rec.payload, 5);
}

void test_build_single_short_record(void)
{
    static const uint8_t type_t = 'T';
    static const uint8_t pay[]  = { 0x02, 'e', 'n', '{', '}' };
    ndef_record_t rec;
    uint8_t       out[32];

    make_record(&rec, NDEF_TNF_WELL_KNOWN, &type_t, 1, pay, sizeof(pay));

    int rc = ndef_build(&rec, 1, out, sizeof(out));

    TEST_ASSERT_GREATER_THAN(0, rc);
    TEST_ASSERT_BITS(0x80, 0x80, out[0]); /* MB set */
    TEST_ASSERT_BITS(0x40, 0x40, out[0]); /* ME set */
    TEST_ASSERT_BITS(0x10, 0x10, out[0]); /* SR set */
}

void test_build_single_long_record(void)
{
    static const uint8_t type_t = 'T';
    uint8_t pay[256];
    memset(pay, 0xCC, sizeof(pay));

    ndef_record_t rec;
    uint8_t       out[512];

    make_record(&rec, NDEF_TNF_WELL_KNOWN, &type_t, 1, pay, sizeof(pay));

    int rc = ndef_build(&rec, 1, out, sizeof(out));

    TEST_ASSERT_GREATER_THAN(0, rc);
    TEST_ASSERT_BITS(0x10, 0x00, out[0]); /* SR NOT set */
    /* payload length at bytes 2-5 */
    TEST_ASSERT_EQUAL_HEX8(0x00, out[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[3]);
    TEST_ASSERT_EQUAL_HEX8(0x01, out[4]);
    TEST_ASSERT_EQUAL_HEX8(0x00, out[5]);
}

/* ------------------------------------------ payload length boundary: 255/256 */

void test_boundary_255_byte_payload(void)
{
    /* Exactly 255 bytes: SR=1 (max short record) */
    static const uint8_t type_t = 'T';
    uint8_t pay[255];
    memset(pay, 0x11, sizeof(pay));

    ndef_record_t rec;
    uint8_t       out[512];
    make_record(&rec, NDEF_TNF_WELL_KNOWN, &type_t, 1, pay, 255);

    int rc = ndef_build(&rec, 1, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, rc);
    TEST_ASSERT_BITS(0x10, 0x10, out[0]); /* SR=1 */
    TEST_ASSERT_EQUAL_HEX8(0xFF, out[2]); /* 1-byte length = 255 */

    ndef_record_t parsed;
    TEST_ASSERT_EQUAL(1, ndef_parse(out, (uint16_t)rc, &parsed, 1));
    TEST_ASSERT_EQUAL(255u, parsed.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(pay, parsed.payload, 255);
}

void test_boundary_256_byte_payload(void)
{
    /* Exactly 256 bytes: forces SR=0 */
    static const uint8_t type_t = 'T';
    uint8_t pay[256];
    memset(pay, 0x22, sizeof(pay));

    ndef_record_t rec;
    uint8_t       out[512];
    make_record(&rec, NDEF_TNF_WELL_KNOWN, &type_t, 1, pay, 256);

    int rc = ndef_build(&rec, 1, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, rc);
    TEST_ASSERT_BITS(0x10, 0x00, out[0]); /* SR=0 */

    ndef_record_t parsed;
    TEST_ASSERT_EQUAL(1, ndef_parse(out, (uint16_t)rc, &parsed, 1));
    TEST_ASSERT_EQUAL(256u, parsed.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(pay, parsed.payload, 256);
}

/* --------------------------------------------------------- multi-record tests */

void test_two_records_mb_me_flags(void)
{
    static const uint8_t type_t = 'T';
    static const uint8_t p1[] = { 0x02, 'e', 'n', 'A' };
    static const uint8_t p2[] = { 0x02, 'e', 'n', 'B' };
    ndef_record_t recs[2];
    uint8_t       out[64];

    make_record(&recs[0], NDEF_TNF_WELL_KNOWN, &type_t, 1, p1, sizeof(p1));
    make_record(&recs[1], NDEF_TNF_WELL_KNOWN, &type_t, 1, p2, sizeof(p2));

    int rc = ndef_build(recs, 2, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, rc);

    /* First record: MB=1, ME=0 */
    TEST_ASSERT_BITS(0xC0, 0x80, out[0]);

    ndef_record_t parsed[2];
    TEST_ASSERT_EQUAL(2, ndef_parse(out, (uint16_t)rc, parsed, 2));
}

void test_three_records_middle_flags(void)
{
    static const uint8_t type_t = 'T';
    static const uint8_t p[] = { 0x02, 'e', 'n', 'X' };
    ndef_record_t recs[3];
    uint8_t       out[128];

    for (int i = 0; i < 3; i++)
        make_record(&recs[i], NDEF_TNF_WELL_KNOWN, &type_t, 1, p, sizeof(p));

    int rc = ndef_build(recs, 3, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, rc);

    ndef_record_t parsed[3];
    TEST_ASSERT_EQUAL(3, ndef_parse(out, (uint16_t)rc, parsed, 3));
}

void test_mixed_short_and_long_records(void)
{
    static const uint8_t type_t = 'T';
    uint8_t long_pay[300];
    memset(long_pay, 0x77, sizeof(long_pay));
    static const uint8_t short_pay[] = { 0x02, 'e', 'n', 'S' };

    ndef_record_t recs[2];
    uint8_t       out[512];

    make_record(&recs[0], NDEF_TNF_WELL_KNOWN, &type_t, 1, long_pay, sizeof(long_pay));
    make_record(&recs[1], NDEF_TNF_WELL_KNOWN, &type_t, 1, short_pay, sizeof(short_pay));

    int rc = ndef_build(recs, 2, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, rc);

    ndef_record_t parsed[2];
    int n = ndef_parse(out, (uint16_t)rc, parsed, 2);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL(300u, parsed[0].payload_len);
    TEST_ASSERT_EQUAL(sizeof(short_pay), parsed[1].payload_len);
    TEST_ASSERT_EQUAL_MEMORY(long_pay,  parsed[0].payload, sizeof(long_pay));
    TEST_ASSERT_EQUAL_MEMORY(short_pay, parsed[1].payload, sizeof(short_pay));
}

void test_max_length_record_then_another(void)
{
    /* Ensure parser stops at ME and does not over-read */
    static const uint8_t type_t = 'T';
    uint8_t pay_a[200], pay_b[10];
    memset(pay_a, 0xAA, sizeof(pay_a));
    memset(pay_b, 0xBB, sizeof(pay_b));

    ndef_record_t recs[2];
    uint8_t       out[512];

    make_record(&recs[0], NDEF_TNF_WELL_KNOWN, &type_t, 1, pay_a, sizeof(pay_a));
    make_record(&recs[1], NDEF_TNF_WELL_KNOWN, &type_t, 1, pay_b, sizeof(pay_b));

    int rc = ndef_build(recs, 2, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, rc);

    ndef_record_t parsed[2];
    TEST_ASSERT_EQUAL(2, ndef_parse(out, (uint16_t)rc, parsed, 2));
    TEST_ASSERT_EQUAL(sizeof(pay_a), parsed[0].payload_len);
    TEST_ASSERT_EQUAL(sizeof(pay_b), parsed[1].payload_len);
    TEST_ASSERT_EQUAL_MEMORY(pay_b, parsed[1].payload, sizeof(pay_b));
}

void test_single_byte_payload(void)
{
    /* Minimal valid record: SR=1, payload_len=1 */
    static const uint8_t type_t = 'T';
    static const uint8_t pay[]  = { 0x42 };
    ndef_record_t rec, parsed;
    uint8_t       out[16];

    make_record(&rec, NDEF_TNF_WELL_KNOWN, &type_t, 1, pay, 1);

    int rc = ndef_build(&rec, 1, out, sizeof(out));
    TEST_ASSERT_GREATER_THAN(0, rc);
    TEST_ASSERT_EQUAL(1, ndef_parse(out, (uint16_t)rc, &parsed, 1));
    TEST_ASSERT_EQUAL(1u, parsed.payload_len);
    TEST_ASSERT_EQUAL_HEX8(0x42, parsed.payload[0]);
}

void test_roundtrip_multiple_variable_length(void)
{
    static const uint8_t type_t = 'T';

    uint8_t p0[1];   memset(p0, 0x01, sizeof(p0));
    uint8_t p1[100]; memset(p1, 0x02, sizeof(p1));
    uint8_t p2[255]; memset(p2, 0x03, sizeof(p2));  /* SR max */
    uint8_t p3[256]; memset(p3, 0x04, sizeof(p3));  /* forces long record */
    uint8_t p4[10];  memset(p4, 0x05, sizeof(p4));

    ndef_record_t recs[5];
    make_record(&recs[0], NDEF_TNF_WELL_KNOWN, &type_t, 1, p0, sizeof(p0));
    make_record(&recs[1], NDEF_TNF_WELL_KNOWN, &type_t, 1, p1, sizeof(p1));
    make_record(&recs[2], NDEF_TNF_WELL_KNOWN, &type_t, 1, p2, sizeof(p2));
    make_record(&recs[3], NDEF_TNF_WELL_KNOWN, &type_t, 1, p3, sizeof(p3));
    make_record(&recs[4], NDEF_TNF_WELL_KNOWN, &type_t, 1, p4, sizeof(p4));

    uint8_t built[1024];
    int built_len = ndef_build(recs, 5, built, sizeof(built));
    TEST_ASSERT_GREATER_THAN(0, built_len);

    ndef_record_t parsed[5];
    TEST_ASSERT_EQUAL(5, ndef_parse(built, (uint16_t)built_len, parsed, 5));

    TEST_ASSERT_EQUAL(sizeof(p0), parsed[0].payload_len);
    TEST_ASSERT_EQUAL(sizeof(p1), parsed[1].payload_len);
    TEST_ASSERT_EQUAL(sizeof(p2), parsed[2].payload_len);
    TEST_ASSERT_EQUAL(sizeof(p3), parsed[3].payload_len);
    TEST_ASSERT_EQUAL(sizeof(p4), parsed[4].payload_len);

    TEST_ASSERT_EQUAL_MEMORY(p0, parsed[0].payload, sizeof(p0));
    TEST_ASSERT_EQUAL_MEMORY(p1, parsed[1].payload, sizeof(p1));
    TEST_ASSERT_EQUAL_MEMORY(p2, parsed[2].payload, sizeof(p2));
    TEST_ASSERT_EQUAL_MEMORY(p3, parsed[3].payload, sizeof(p3));
    TEST_ASSERT_EQUAL_MEMORY(p4, parsed[4].payload, sizeof(p4));
}

/* --------------------------------------------------------------------- main */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_single_short_record);
    RUN_TEST(test_parse_single_long_record);
    RUN_TEST(test_build_single_short_record);
    RUN_TEST(test_build_single_long_record);

    RUN_TEST(test_boundary_255_byte_payload);
    RUN_TEST(test_boundary_256_byte_payload);

    RUN_TEST(test_two_records_mb_me_flags);
    RUN_TEST(test_three_records_middle_flags);
    RUN_TEST(test_mixed_short_and_long_records);
    RUN_TEST(test_max_length_record_then_another);
    RUN_TEST(test_single_byte_payload);

    RUN_TEST(test_roundtrip_multiple_variable_length);

    return UNITY_END();
}
