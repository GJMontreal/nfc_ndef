#ifndef ST25DV_HAL_H
#define ST25DV_HAL_H

#include <stdint.h>

/**
 * I2C read callback.
 *
 * @param dev_addr  7-bit I2C device address
 * @param mem_addr  16-bit memory address
 * @param buf       destination buffer
 * @param len       number of bytes to read
 * @param ctx       caller-supplied context pointer
 * @return          0 on success, non-zero on error
 */
typedef int (*st25dv_i2c_read_fn)(uint8_t dev_addr, uint16_t mem_addr,
                                   uint8_t *buf, uint16_t len, void *ctx);

/**
 * I2C write callback.
 *
 * @param dev_addr  7-bit I2C device address
 * @param mem_addr  16-bit memory address
 * @param buf       source buffer
 * @param len       number of bytes to write
 * @param ctx       caller-supplied context pointer
 * @return          0 on success, non-zero on error
 */
typedef int (*st25dv_i2c_write_fn)(uint8_t dev_addr, uint16_t mem_addr,
                                    const uint8_t *buf, uint16_t len, void *ctx);

typedef struct {
    st25dv_i2c_read_fn  read;
    st25dv_i2c_write_fn write;
    void               *ctx;
} st25dv_hal_t;

#endif /* ST25DV_HAL_H */
