#include "mpu6050.h"

// -----------------------------
// Função auxiliar para configurar I2C
// -----------------------------
static void mpu6050_setup_i2c(i2c_inst_t *i2c, uint8_t sda, uint8_t scl){
    i2c_init(i2c, 400 * 1000); // 400 kHz
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

// -----------------------------
// Escrita e leitura I2C auxiliares
// -----------------------------
static inline void mpu6050_write(MPU6050 *sensor, uint8_t reg, uint8_t data){
    uint8_t buf[2] = {reg, data};
    i2c_write_blocking(sensor->i2c, sensor->addr, buf, 2, false);
}

static inline void mpu6050_read_bytes(MPU6050 *sensor, uint8_t reg, uint8_t *buf, size_t len){
    i2c_write_blocking(sensor->i2c, sensor->addr, &reg, 1, true);
    i2c_read_blocking(sensor->i2c, sensor->addr, buf, len, false);
}

// -----------------------------
// Inicialização padrão
// -----------------------------
bool mpu6050_init(MPU6050 *sensor, i2c_inst_t *i2c, uint8_t sda, uint8_t scl){
    sensor->i2c = i2c;
    sensor->addr = MPU6050_ADDR_DEFAULT;
    sensor->accel_range = ACCEL_SENS_2G; // default ±2g

    // Configura pinos e inicializa I2C
    mpu6050_setup_i2c(i2c, sda, scl);

    // Reset e configuração inicial
    mpu6050_reset(sensor);
    mpu6050_set_accel_range(sensor, sensor->accel_range);

    return true;
}

// -----------------------------
// Inicialização costumizada
// -----------------------------
bool mpu6050_init_custom(MPU6050 *sensor, i2c_inst_t *i2c, uint8_t sda, uint8_t scl, uint8_t addr, uint16_t accel_range){
    sensor->i2c = i2c;
    sensor->addr = addr;
    sensor->accel_range = accel_range;

    // Configura pinos e inicializa I2C
    mpu6050_setup_i2c(i2c, sda, scl);

    // Reset e configuração inicial
    mpu6050_reset(sensor);
    mpu6050_set_accel_range(sensor, sensor->accel_range);

    return true;
}

// -----------------------------
// Reset
// -----------------------------
void mpu6050_reset(MPU6050 *sensor){
    mpu6050_write(sensor, 0x6B, 0x80); // PWR_MGMT_1 reset
    sleep_ms(100);
    mpu6050_write(sensor, 0x6B, 0x00); // wake up
    sleep_ms(10);
}

// -----------------------------
// Range do acelerômetro
// -----------------------------
uint16_t mpu6050_get_accel_range(MPU6050 *sensor) {
    uint8_t reg = 0x1C;
    uint8_t val;
    mpu6050_read_bytes(sensor, reg, &val, 1);
    uint8_t range = (val >> 3) & 0x03;

    switch (range) {
        case 0: return ACCEL_SENS_2G;
        case 1: return ACCEL_SENS_4G;
        case 2: return ACCEL_SENS_8G;
        case 3: return ACCEL_SENS_16G;
        default: return ACCEL_SENS_2G; // fallback
    }
}


void mpu6050_set_accel_range(MPU6050 *sensor, uint16_t sensibility) {
    uint8_t range;

    if (sensibility == ACCEL_SENS_2G) {
        range = 0;
        sensor->accel_range = ACCEL_SENS_2G;
    } else if (sensibility == ACCEL_SENS_4G) {
        range = 1;
        sensor->accel_range = ACCEL_SENS_4G;
    } else if (sensibility == ACCEL_SENS_8G) {
        range = 2;
        sensor->accel_range = ACCEL_SENS_8G;
    } else if (sensibility == ACCEL_SENS_16G) {
        range = 3;
        sensor->accel_range = ACCEL_SENS_16G;
    } else {
        range = 0;
        sensor->accel_range = ACCEL_SENS_2G; // default
    }

    mpu6050_write(sensor, 0x1C, range << 3);
}

// -----------------------------
// Leitura (Accel, Gyro, Temp)
// -----------------------------
bool mpu6050_read(MPU6050 *sensor){
    uint8_t buffer[14];
    mpu6050_read_bytes(sensor, 0x3B, buffer, 14);

    // Accel
    int16_t raw_ax = (buffer[0] << 8) | buffer[1];
    int16_t raw_ay = (buffer[2] << 8) | buffer[3];
    int16_t raw_az = (buffer[4] << 8) | buffer[5];

    sensor->accel[0] = raw_ax / sensor->accel_range;
    sensor->accel[1] = raw_ay / sensor->accel_range;
    sensor->accel[2] = raw_az / sensor->accel_range;

    // Temp
    int16_t raw_temp = (buffer[6] << 8) | buffer[7];
    sensor->temperature = (raw_temp / 340.0f) + 36.53f;

    // Gyro
    int16_t raw_gx = (buffer[8] << 8) | buffer[9];
    int16_t raw_gy = (buffer[10] << 8) | buffer[11];
    int16_t raw_gz = (buffer[12] << 8) | buffer[13];

    sensor->gyro[0] = raw_gx / 131.0f; // ±250 °/s default
    sensor->gyro[1] = raw_gy / 131.0f;
    sensor->gyro[2] = raw_gz / 131.0f;

    return true;
}

// -----------------------------
// Leitura de dados sem tratamento(cru) (Accel, Gyro, Temp)
// -----------------------------
bool mpu6050_read_raw(MPU6050 *sensor){
    uint8_t buffer[6];
    uint8_t reg;

    // Acelerômetro
    reg = 0x3B;
    i2c_write_blocking(sensor->i2c, sensor->addr, &reg, 1, true);
    i2c_read_blocking(sensor->i2c, sensor->addr, buffer, 6, false);
    for (int i = 0; i < 3; i++){
        sensor->accel_raw[i] = (buffer[2 * i] << 8) | buffer[2 * i + 1];
    }

    // Giroscópio
    reg = 0x43;
    i2c_write_blocking(sensor->i2c, sensor->addr, &reg, 1, true);
    i2c_read_blocking(sensor->i2c, sensor->addr, buffer, 6, false);
    for (int i = 0; i < 3; i++){
        sensor->gyro_raw[i] = (buffer[2 * i] << 8) | buffer[2 * i + 1];
    }

    // Temperatura
    reg = 0x41;
    i2c_write_blocking(sensor->i2c, sensor->addr, &reg, 1, true);
    i2c_read_blocking(sensor->i2c, sensor->addr, buffer, 2, false);
    sensor->temp_raw = (buffer[0] << 8) | buffer[1];

    return true;
}
