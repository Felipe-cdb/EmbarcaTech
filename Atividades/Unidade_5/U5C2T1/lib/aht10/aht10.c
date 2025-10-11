// Adaptada por Felipe Correia das seguintes bibliotecas 
// Biblioteca bh1750 por Adafruit Industries
// https://github.com/adafruit/Adafruit_AHTX0/tree/master
// Biblioteca bh1750 por Juliano Oliveira
// https://github.com/jrfo-hwit/hlab/tree/main/firmware/c_cpp/examples/3_aht10_i2c_uart0
// Biblioteca TESTE_bh1750_BITDOGLAB por por Antonio Sergio Castro de Carvalho Junior
// https://github.com/ASCCJR/TESTE_aht10_BITDOGLAB

#include "aht10.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <math.h>   // para log()
#include <stdio.h>  // debug opcional

// -----------------------------
// Funções privadas auxiliares
// -----------------------------
static bool aht10_write_cmd(AHT10 *sensor, const uint8_t *cmd, size_t len, bool nostop) {
    int ret = i2c_write_blocking(sensor->i2c, sensor->addr, cmd, len, nostop);
    return (ret >= 0);
}

static bool aht10_read_bytes(AHT10 *sensor, uint8_t *buf, size_t len) {
    int ret = i2c_read_blocking(sensor->i2c, sensor->addr, buf, len, false);
    return (ret >= 0);
}

// -----------------------------
// Inicialização
// -----------------------------

static void aht10_setup_i2c(i2c_inst_t* i2c, uint8_t sda, uint8_t scl){
    // Inicializa o I2C. 100kHz é uma velocidade segura para depuração.
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

bool aht10_init(AHT10* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl) {
    return aht10_init_custom(sensor, i2c, sda, scl, AHT10_ADDRESS_DEFAULT);
    return true;
}

bool aht10_init_custom(AHT10* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl, uint8_t addr) {
    sensor->i2c = i2c;
    sensor->sda = sda;
    sensor->scl = scl;
    sensor->addr = addr;
    sensor->temperature = 0.0f;
    sensor->humidity = 0.0f;

    // Configura pinos e inicializa I2C
    aht10_setup_i2c(sensor->i2c, sensor->sda, sensor->scl);

    // Reset do sensor
    if (!aht10_write_cmd(sensor, &CMD_RESET, 1, false)) {
        return false;
    }
    sleep_ms(20);

    // Envia comando de inicialização (calibração)
    if (!aht10_write_cmd(sensor, CMD_INIT, sizeof(CMD_INIT), false)) {
        return false;
    }
    sleep_ms(20);

    // Verifica calibração
    uint8_t status;
    if (!aht10_read_bytes(sensor, &status, 1)) {
        return false;
    }
    if ((status & AHT10_STATUS_CALIBRATED) == 0) {
        // bit 3 = 1 → calibrado
        return false;
    }

    return true;
}

// -----------------------------
// Leitura única (atualiza struct)
// -----------------------------
bool aht10_read(AHT10 *sensor) {
    // Dispara medição
    if (!aht10_write_cmd(sensor, CMD_MEASURE, sizeof(CMD_MEASURE), false)) {
        return false;
    }

    sleep_ms(80); // tempo de conversão típico

    // Lê resposta
    uint8_t buf[6];
    if (!aht10_read_bytes(sensor, buf, 6)) {
        return false;
    }

    if (buf[0] & AHT10_STATUS_BUSY) return false;  // ocupado
    if ((buf[0] & AHT10_STATUS_CALIBRATED) == 0) return false; // não calibrado

    // Umidade
    uint32_t raw_hum = ((uint32_t)buf[1] << 12) |
                       ((uint32_t)buf[2] << 4) |
                       ((buf[3] & 0xF0) >> 4);
    sensor->humidity = ((float)raw_hum / 1048576.0f) * 100.0f;

    // Temperatura
    uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) |
                        ((uint32_t)buf[4] << 8) |
                        (uint32_t)buf[5];
    sensor->temperature = (((float)raw_temp / 1048576.0f) * 200.0f) - 50.0f;

    return true;
}

// -----------------------------
// Getters (estilo simplificado)
// -----------------------------
float aht10_get_temperature(AHT10 *sensor) {
    if (aht10_read(sensor)) {
        return sensor->temperature;
    }
    return -1000.0f; // valor de erro
}

float aht10_get_humidity(AHT10 *sensor) {
    if (aht10_read(sensor)) {
        return sensor->humidity;
    }
    return -1.0f; // valor de erro
}

// -----------------------------
// Ponto de orvalho
// -----------------------------
float aht10_get_dew_point(AHT10 *sensor) {
    if (!aht10_read(sensor)) {
        return -1000.0f; // erro
    }

    const float a = 17.27f;
    const float b = 237.7f;
    float gamma = (a * sensor->temperature) / (b + sensor->temperature) +
                  log(sensor->humidity / 100.0f);
    float dew_point = (b * gamma) / (a - gamma);
    return dew_point;
}
