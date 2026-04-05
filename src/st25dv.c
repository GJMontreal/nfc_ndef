#include "st25dv.h"
#include <string.h>

int st25dv_init(st25dv_t *dev, const st25dv_hal_t *hal)
{
    uint8_t cc[ST25DV_CC_LEN];

    if (!dev || !hal || !hal->read || !hal->write)
        return ST25DV_ERR_PARAM;

    dev->hal = *hal;

    if (dev->hal.read(ST25DV_ADDR_USER_MEM, ST25DV_CC_ADDR,
                      cc, ST25DV_CC_LEN, dev->hal.ctx) != 0)
        return ST25DV_ERR_HAL;

    if (cc[0] != ST25DV_CC_MAGIC)
        return ST25DV_ERR_NO_NDEF;

    dev->mem_size_bytes = ST25DV_CC_MLEN_TO_BYTES(cc[2]);

    return ST25DV_OK;
}

int st25dv_read(const st25dv_t *dev, uint16_t addr, uint8_t *buf, uint16_t len)
{
    if (!dev || !buf || len == 0)
        return ST25DV_ERR_PARAM;

    if (dev->hal.read(ST25DV_ADDR_USER_MEM, addr, buf, len, dev->hal.ctx) != 0)
        return ST25DV_ERR_HAL;

    return ST25DV_OK;
}

int st25dv_write(const st25dv_t *dev, uint16_t addr, const uint8_t *buf, uint16_t len)
{
    if (!dev || !buf || len == 0)
        return ST25DV_ERR_PARAM;

    if (dev->hal.write(ST25DV_ADDR_USER_MEM, addr, buf, len, dev->hal.ctx) != 0)
        return ST25DV_ERR_HAL;

    return ST25DV_OK;
}

int st25dv_ndef_read(const st25dv_t *dev, uint8_t *buf, uint16_t buf_len,
                     uint16_t *out_len)
{
    uint16_t avail;

    if (!dev || !buf || buf_len == 0 || !out_len)
        return ST25DV_ERR_PARAM;

    if (dev->mem_size_bytes <= ST25DV_NDEF_START_ADDR)
        return ST25DV_ERR_OVERFLOW;

    avail = dev->mem_size_bytes - ST25DV_NDEF_START_ADDR;
    if (buf_len < avail)
        avail = buf_len;

    if (dev->hal.read(ST25DV_ADDR_USER_MEM, ST25DV_NDEF_START_ADDR,
                      buf, avail, dev->hal.ctx) != 0)
        return ST25DV_ERR_HAL;

    *out_len = avail;
    return ST25DV_OK;
}

int st25dv_ndef_write(const st25dv_t *dev, const uint8_t *buf, uint16_t len)
{
    uint16_t avail;

    if (!dev || !buf || len == 0)
        return ST25DV_ERR_PARAM;

    if (dev->mem_size_bytes <= ST25DV_NDEF_START_ADDR)
        return ST25DV_ERR_OVERFLOW;

    avail = dev->mem_size_bytes - ST25DV_NDEF_START_ADDR;
    if (len > avail)
        return ST25DV_ERR_OVERFLOW;

    if (dev->hal.write(ST25DV_ADDR_USER_MEM, ST25DV_NDEF_START_ADDR,
                       buf, len, dev->hal.ctx) != 0)
        return ST25DV_ERR_HAL;

    return ST25DV_OK;
}
