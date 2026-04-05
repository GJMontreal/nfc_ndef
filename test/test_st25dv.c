#include "unity.h"
#include "st25dv.h"
#include "ndef.h"
#include "ndef_tlv.h"
#include "mock_hal.h"
#include <string.h>

static mock_hal_ctx_t g_ctx;
static st25dv_hal_t   g_hal;

void setUp(void)
{
    mock_hal_init(&g_ctx);
    g_hal = mock_hal_get(&g_ctx);
}

void tearDown(void) {}

/* --------------------------------------------------------------- init tests */

void test_init_valid_cc(void)
{
    st25dv_t dev;
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_init(&dev, &g_hal));
    /* MLEN=0x3F → (0x3F+1)*8 = 512 bytes */
    TEST_ASSERT_EQUAL(512u, dev.mem_size_bytes);
}

void test_init_bad_magic(void)
{
    g_ctx.eeprom[0] = 0x00;  /* corrupt the CC magic byte */
    st25dv_t dev;
    TEST_ASSERT_EQUAL(ST25DV_ERR_NO_NDEF, st25dv_init(&dev, &g_hal));
}

void test_init_null_hal(void)
{
    st25dv_t dev;
    TEST_ASSERT_EQUAL(ST25DV_ERR_PARAM, st25dv_init(&dev, NULL));
}

void test_init_null_read_fn(void)
{
    st25dv_hal_t bad_hal = g_hal;
    bad_hal.read = NULL;
    st25dv_t dev;
    TEST_ASSERT_EQUAL(ST25DV_ERR_PARAM, st25dv_init(&dev, &bad_hal));
}

/* -------------------------------------------------- raw read / write tests */

void test_raw_read_write(void)
{
    st25dv_t      dev;
    uint8_t       wbuf[] = { 0x12, 0x34, 0x56 };
    uint8_t       rbuf[3] = { 0 };

    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_init(&dev, &g_hal));
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_write(&dev, 0x0010, wbuf, sizeof(wbuf)));
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_read(&dev,  0x0010, rbuf, sizeof(rbuf)));
    TEST_ASSERT_EQUAL_MEMORY(wbuf, rbuf, sizeof(wbuf));
}

void test_raw_write_read_at_ndef_start(void)
{
    st25dv_t dev;
    uint8_t  wbuf[] = { 0xAA, 0xBB };
    uint8_t  rbuf[2] = { 0 };

    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_init(&dev, &g_hal));
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_write(&dev, ST25DV_NDEF_START_ADDR, wbuf, sizeof(wbuf)));
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_read(&dev,  ST25DV_NDEF_START_ADDR, rbuf, sizeof(rbuf)));
    TEST_ASSERT_EQUAL_MEMORY(wbuf, rbuf, sizeof(wbuf));
}

/* -------------------------------------------------- NDEF read / write tests */

void test_ndef_write_read_roundtrip(void)
{
    st25dv_t dev;
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_init(&dev, &g_hal));

    /* Build a minimal NDEF TLV with one text record */
    static const uint8_t type_t = 'T';
    static const uint8_t pay[]  = { 0x02, 'e', 'n', '{', '"', 'v', '"', ':', '1', '}' };
    ndef_record_t rec;
    rec.tnf         = NDEF_TNF_WELL_KNOWN;
    rec.type        = &type_t;
    rec.type_len    = 1;
    rec.id          = NULL;
    rec.id_len      = 0;
    rec.payload     = pay;
    rec.payload_len = sizeof(pay);

    uint8_t ndef_buf[64], tlv_buf[80];
    int ndef_len = ndef_build(&rec, 1, ndef_buf, sizeof(ndef_buf));
    TEST_ASSERT_GREATER_THAN(0, ndef_len);

    int tlv_len = ndef_tlv_wrap(ndef_buf, (uint16_t)ndef_len, tlv_buf, sizeof(tlv_buf));
    TEST_ASSERT_GREATER_THAN(0, tlv_len);

    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_ndef_write(&dev, tlv_buf, (uint16_t)tlv_len));

    uint8_t read_buf[80];
    uint16_t read_len = 0;
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_ndef_read(&dev, read_buf, sizeof(read_buf), &read_len));

    /* Unwrap and parse */
    const uint8_t *unwrapped = NULL;
    uint16_t       unwrapped_len = 0;
    TEST_ASSERT_EQUAL(NDEF_OK,
        ndef_tlv_unwrap(read_buf, read_len, &unwrapped, &unwrapped_len));

    ndef_record_t parsed;
    TEST_ASSERT_EQUAL(1, ndef_parse(unwrapped, unwrapped_len, &parsed, 1));
    TEST_ASSERT_EQUAL(sizeof(pay), parsed.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(pay, parsed.payload, sizeof(pay));
}

void test_ndef_write_overflow(void)
{
    st25dv_t dev;
    TEST_ASSERT_EQUAL(ST25DV_OK, st25dv_init(&dev, &g_hal));

    /* mem_size_bytes=512, NDEF region=508 bytes. Try to write 600. */
    uint8_t big[600];
    memset(big, 0, sizeof(big));
    TEST_ASSERT_EQUAL(ST25DV_ERR_OVERFLOW,
        st25dv_ndef_write(&dev, big, sizeof(big)));
}

/* --------------------------------------------------------------------- main */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_valid_cc);
    RUN_TEST(test_init_bad_magic);
    RUN_TEST(test_init_null_hal);
    RUN_TEST(test_init_null_read_fn);

    RUN_TEST(test_raw_read_write);
    RUN_TEST(test_raw_write_read_at_ndef_start);

    RUN_TEST(test_ndef_write_read_roundtrip);
    RUN_TEST(test_ndef_write_overflow);

    return UNITY_END();
}
