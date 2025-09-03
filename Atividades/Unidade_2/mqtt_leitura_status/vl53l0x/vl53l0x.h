#ifndef VL53L0X_C_H
#define VL53L0X_C_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
    uint16_t io_timeout;
    bool did_timeout;
} VL53L0X;

bool vl53l0x_init(VL53L0X *dev, i2c_inst_t *i2c, uint8_t address);
int16_t vl53l0x_read_distance(VL53L0X *dev);

#endif
