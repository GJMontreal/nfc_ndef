#include "mock_hal.h"
#include <string.h>

static int mock_read(uint8_t dev_addr, uint16_t mem_addr,
                     uint8_t *buf, uint16_t len, void *ctx)
{
    mock_hal_ctx_t *m = (mock_hal_ctx_t *)ctx;
    (void)dev_addr;

    if ((uint32_t)mem_addr + len > MOCK_EEPROM_SIZE)
        return -1;

    memcpy(buf, &m->eeprom[mem_addr], len);
    return 0;
}

static int mock_write(uint8_t dev_addr, uint16_t mem_addr,
                      const uint8_t *buf, uint16_t len, void *ctx)
{
    mock_hal_ctx_t *m = (mock_hal_ctx_t *)ctx;
    (void)dev_addr;

    if ((uint32_t)mem_addr + len > MOCK_EEPROM_SIZE)
        return -1;

    memcpy(&m->eeprom[mem_addr], buf, len);
    return 0;
}

void mock_hal_init(mock_hal_ctx_t *ctx)
{
    memset(ctx->eeprom, 0x00, MOCK_EEPROM_SIZE);

    ctx->eeprom[0] = 0xE2;  /* CC magic */
    ctx->eeprom[1] = 0x40;  /* version 1.0, read/write */
    ctx->eeprom[2] = 0x3F;  /* MLEN: (0x3F+1)*8 = 512 bytes */
    ctx->eeprom[3] = 0x00;  /* access conditions */
}

st25dv_hal_t mock_hal_get(mock_hal_ctx_t *ctx)
{
    st25dv_hal_t hal;
    hal.read  = mock_read;
    hal.write = mock_write;
    hal.ctx   = ctx;
    return hal;
}
