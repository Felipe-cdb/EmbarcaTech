#include "vl53l0x.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include <stdio.h>

#define VL53L0X_REG_RESULT_RANGE_STATUS 0x14
#define VL53L0X_REG_SYSRANGE_START 0x00

static void write_reg(VL53L0X *dev, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(dev->i2c, dev->address, buf, 2, false);
}

static uint8_t read_reg(VL53L0X *dev, uint8_t reg) {
    uint8_t value;
    i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true);
    i2c_read_blocking(dev->i2c, dev->address, &value, 1, false);
    return value;
}

bool vl53l0x_init(VL53L0X *dev, i2c_inst_t *i2c, uint8_t address) {
    dev->i2c = i2c;
    dev->address = address;
    dev->io_timeout = 500;
    dev->did_timeout = false;

    // Verifica comunicação básica
    uint8_t id = read_reg(dev, 0xC0); // Modelo ID típico
    if (id == 0 || id == 0xFF) return false;

    // Comando de start simples (mais detalhes requerem init completo)
    write_reg(dev, VL53L0X_REG_SYSRANGE_START, 0x01);
    return true;
}

int16_t vl53l0x_read_distance(VL53L0X *dev) {
    // Inicia uma nova medição
    write_reg(dev, VL53L0X_REG_SYSRANGE_START, 0x01);

    // Espera a medição ser finalizada (pode levar alguns milissegundos)
    uint16_t timeout = dev->io_timeout;
    while (read_reg(dev, VL53L0X_REG_RESULT_RANGE_STATUS) & 0x01) {
        if (timeout == 0) {
            dev->did_timeout = true;
            return -1;
        }
        sleep_ms(1);
        timeout--;
    }

    uint8_t reg = VL53L0X_REG_RESULT_RANGE_STATUS + 10;
    i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true);

    uint8_t buffer[2] = {0};
    if (i2c_read_blocking(dev->i2c, dev->address, buffer, 2, false) != 2) {
        return -1;
    }

    uint16_t distance = ((uint16_t)buffer[0] << 8) | buffer[1];
    return distance;
}