#ifndef ST25DV_H
#define ST25DV_H

#include <stdint.h>
#include "st25dv_hal.h"

/* 7-bit I2C addresses */
#define ST25DV_ADDR_USER_MEM    0x53U   /* EEPROM user memory */
#define ST25DV_ADDR_SYS_CFG     0x57U   /* System configuration registers */

/* Capability Container */
#define ST25DV_CC_ADDR          0x0000U
#define ST25DV_CC_LEN           4U
#define ST25DV_CC_MAGIC         0xE2U
#define ST25DV_NDEF_START_ADDR  0x0004U /* Immediately after the 4-byte CC */

/*
 * CC byte 2 (MLEN): user memory size per T5T spec = (MLEN + 1) * 8 bytes.
 * Valid for ST25DV04K (MLEN=0x3F → 512 B) and ST25DV16K (MLEN=0xFF → 2048 B).
 * ST25DV64K uses an Extended CC — this driver supports standard 4-byte CC only.
 * Verify MLEN encoding against the datasheet before use.
 */
#define ST25DV_CC_MLEN_TO_BYTES(mlen)   (((uint16_t)(mlen) + 1U) * 8U)

/* Error codes */
#define ST25DV_OK               0
#define ST25DV_ERR_HAL         -1
#define ST25DV_ERR_NO_NDEF     -2
#define ST25DV_ERR_PARAM       -3
#define ST25DV_ERR_OVERFLOW    -4

typedef struct {
    st25dv_hal_t hal;
    uint16_t     mem_size_bytes;  /* total user memory, derived from CC MLEN */
} st25dv_t;

/**
 * Initialise the driver. Reads the Capability Container to confirm NDEF
 * support and derive memory size. Must be called before any other function.
 *
 * @return ST25DV_OK or a negative error code.
 */
int st25dv_init(st25dv_t *dev, const st25dv_hal_t *hal);

/**
 * Read raw bytes from user EEPROM.
 */
int st25dv_read(const st25dv_t *dev, uint16_t addr, uint8_t *buf, uint16_t len);

/**
 * Write raw bytes to user EEPROM.
 * Write cycles are limited; avoid unnecessary writes.
 */
int st25dv_write(const st25dv_t *dev, uint16_t addr, const uint8_t *buf, uint16_t len);

/**
 * Read the NDEF TLV region (starting at ST25DV_NDEF_START_ADDR) into buf.
 * Sets *out_len to the number of bytes read.
 */
int st25dv_ndef_read(const st25dv_t *dev, uint8_t *buf, uint16_t buf_len,
                     uint16_t *out_len);

/**
 * Write a complete NDEF TLV region (including TLV framing and terminator).
 */
int st25dv_ndef_write(const st25dv_t *dev, const uint8_t *buf, uint16_t len);

#endif /* ST25DV_H */
