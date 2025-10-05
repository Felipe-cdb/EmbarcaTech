// Adaptada por Felipe Correia das seguintes bibliotecas 
// Biblioteca bh1750 por dernasherbrezon
// https://github.com/dernasherbrezon/bh1750
// Biblioteca Biblioteca TESTE_bh1750_BITDOGLAB por por Antonio Sergio Castro de Carvalho Junior
// https://github.com/ASCCJR/TESTE_bh1750_BITDOGLAB/tree/main


#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bh1750.h"

// Envia um comando simples (1 byte) para o sensor
static int bh1750_send_cmd(BH1750* sensor, uint8_t cmd) {
    return i2c_write_blocking(sensor->i2c, sensor->addr, &cmd, 1, false);
}

static void bh1750_setup_i2c(i2c_inst_t* i2c, uint8_t sda, uint8_t scl){
    // Inicializa o I2C. 100kHz é uma velocidade segura para depuração.
    i2c_init(i2c, 100 * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
}

bool bh1750_init(BH1750* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl) {
    return bh1750_init_custom(sensor, i2c, sda, scl, BH1750_ADDRESS_DEFAULT, BH1750_CONTINUOUSLY_H_RES_MODE);
    return true;
}

bool bh1750_init_custom(BH1750* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t mode) {
    sensor->i2c = i2c;
    sensor->sda = sda;
    sensor->scl = scl;
    sensor->addr = addr;
    sensor->mode = mode;

    // Configura pinos e inicializa I2C
    bh1750_setup_i2c(sensor->i2c, sensor->sda, sensor->scl);

    bh1750_send_cmd(sensor, BH1750_POWER_ON);
    sleep_ms(10);

    bh1750_send_cmd(sensor, BH1750_RESET);
    sleep_ms(10);

    bh1750_send_cmd(sensor, sensor->mode);
    sleep_ms(10);

    return true;
}

float bh1750_read_lux(BH1750* sensor) {
    uint8_t data[2];

    // Se for one-time mode, envia comando e espera
    if (sensor->mode == BH1750_ONE_TIME_H_RES_MODE ||
        sensor->mode == BH1750_ONE_TIME_H_RES_MODE2 ||
        sensor->mode == BH1750_ONE_TIME_L_RES_MODE) {

        bh1750_send_cmd(sensor, sensor->mode);
        
        // Tempo de conversão depende do modo
        if (sensor->mode == BH1750_ONE_TIME_L_RES_MODE)
            sleep_ms(24);
        else
            sleep_ms(180);
    }

    int ret = i2c_read_blocking(sensor->i2c, sensor->addr, data, 2, false);
    if (ret < 0) return -1.0f;

    uint16_t raw = (data[0] << 8) | data[1];
    return raw / 1.2f;
}

void bh1750_set_mode(BH1750* sensor, uint8_t mode) {
    // Atualiza o modo do sensor
    sensor->mode = mode;

    // Envia o comando de reset para o sensor (precisa disso em alguns casos de mudança de modo)
    bh1750_send_cmd(sensor, BH1750_RESET);
    sleep_ms(10);

    // Envia o comando para configurar o novo modo
    bh1750_send_cmd(sensor, sensor->mode);
    sleep_ms(10);
}

void bh1750_set_measurement_time(BH1750* sensor, uint8_t mt) {
    if (mt < 31) mt = 31;
    if (mt > 254) mt = 254;

    // O tempo de medição é configurado em dois comandos:
    // 01000_MT[7,6,5]
    uint8_t high = 0x40 | (mt >> 5);
    // 011_MT[4,3,2,1,0]
    uint8_t low  = 0x60 | (mt & 0x1F);

    bh1750_send_cmd(sensor, high);
    bh1750_send_cmd(sensor, low);
}