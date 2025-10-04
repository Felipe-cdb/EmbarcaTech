#ifndef MPU6050_H
#define MPU6050_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// -----------------------------
// Constantes de sensibilidade
// -----------------------------
#define ACCEL_SENS_2G  16384.0f
#define ACCEL_SENS_4G  8192.0f
#define ACCEL_SENS_8G  4096.0f
#define ACCEL_SENS_16G 2048.0f

// Endereço padrão do MPU6050
#define MPU6050_ADDR_DEFAULT 0x68

// Endereços alternativos (AD0 ligado ao VCC)
#define MPU6050_ADDR_ALTERNATE 0x69

// -----------------------------
// Estrutura do sensor
// -----------------------------
typedef struct {
    i2c_inst_t *i2c;
    uint8_t sda, scl;
    uint8_t addr;

    // Valores convertidos
    float accel[3];      // valores em g
    float gyro[3];       // valores em °/s
    float temperature;   // temperatura em °C

    // Valores crus
    int16_t accel_raw[3];
    int16_t gyro_raw[3];
    int16_t temp_raw;
    
    // range configurado (0=±2g, 1=±4g, 2=±8g, 3=±16g)
    uint16_t accel_range;
} MPU6050;


// -----------------------------
// Funções principais
// -----------------------------
bool mpu6050_init(MPU6050 *sensor, i2c_inst_t *i2c, uint8_t sda, uint8_t scl);
bool mpu6050_init_custom(MPU6050 *sensor, i2c_inst_t *i2c, uint8_t sda, uint8_t scl, uint8_t addr, uint16_t accel_range);
void mpu6050_reset(MPU6050 *sensor);
uint16_t mpu6050_get_accel_range(MPU6050 *sensor);
void mpu6050_set_accel_range(MPU6050 *sensor, uint16_t sensibility);
bool mpu6050_read(MPU6050 *sensor);
bool mpu6050_read_raw(MPU6050 *sensor);

#endif // MPU6050_H
