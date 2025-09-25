// Adaptada por Felipe Correia das seguintes bibliotecas 
// Biblioteca bh1750 por dernasherbrezon
// https://github.com/dernasherbrezon/bh1750
// Biblioteca i2c por Raspberry Pi
// TESTE_bh1750_BITDOGLAB por Antonio Sergio Castro de Carvalho Junior
// https://github.com/ASCCJR/TESTE_bh1750_BITDOGLAB/tree/main

#ifndef BH1750_H
#define BH1750_H

#include "hardware/i2c.h"

// Endereços possíveis do BH1750 no barramento I2C
#define BH1750_ADDRESS_DEFAULT 0x23   // Endereço padrão do sensor
#define BH1750_ADDRESS_ALT     0x5C   // Endereço alternativo (quando A0 é conectado a VCC)

// Comandos de controle do sensor
#define BH1750_POWER_ON		0x01      // Liga o sensor (modo POWER ON)
#define BH1750_RESET		    0x07      // Reseta o registrador de dados (apenas válido se o sensor estiver ligado)

// Modos de operação contínuos (medição repetida automaticamente)
#define BH1750_CONTINUOUSLY_H_RES_MODE  0x10  // Alta resolução (~1 lux), leitura contínua
#define BH1750_CONTINUOUSLY_H_RES_MODE2 0x11  // Alta resolução (~0.5 lux), leitura contínua
#define BH1750_CONTINUOUSLY_L_RES_MODE  0x13  // Baixa resolução (~4 lux), leitura contínua

// Modos de operação “uma vez” (one-time, sensor faz a medição e volta para POWER DOWN)
#define BH1750_ONE_TIME_H_RES_MODE      0x20  // Alta resolução (~1 lux), mede uma vez
#define BH1750_ONE_TIME_H_RES_MODE2     0x21  // Alta resolução (~0.5 lux), mede uma vez
#define BH1750_ONE_TIME_L_RES_MODE      0x23  // Baixa resolução (~4 lux), mede uma vez

// Estrutura de um sensor BH1750
typedef struct {
    i2c_inst_t *i2c;
    uint8_t addr;
    uint8_t mode;
} BH1750;

/**
 * @brief Inicializa o sensor BH1750.
 * 
 * @param i2c  Instância do I2C (ex: i2c0).
 * @param addr Endereço I2C do sensor (0x23 ou 0x5C).
 * @param mode Modo de operação (ex: BH1750_CONTINUOUSLY_H_RES_MODE).
 * 
 * Caso queira inicializar com valores padrão, use a versão simplificada.
 */
void bh1750_init(BH1750 *sensor, i2c_inst_t *i2c, uint8_t addr, uint8_t mode);

/**
 * @brief Inicialização simplificada (usa endereço 0x23 e modo contínuo H-RES).
 */
static inline void bh1750_init_default(BH1750 *sensor, i2c_inst_t *i2c) {
    bh1750_init(sensor, i2c, BH1750_ADDRESS_DEFAULT, BH1750_CONTINUOUSLY_H_RES_MODE);
}

/**
 * @brief Lê o valor de luminosidade em Lux.
 * 
 * @param i2c  Instância do I2C.
 * @param addr Endereço do sensor.
 * @return Valor da luminosidade em Lux, ou -1.0f em caso de erro.
 */
float bh1750_read_lux(BH1750 *sensor);

/**
 * @brief Ajusta o tempo de medição (sensibilidade).
 * 
 * @param i2c  Instância do I2C.
 * @param addr Endereço do sensor.
 * @param mt   Tempo de medição (31–254, default = 69).
 */
void bh1750_set_measurement_time(BH1750 *sensor, uint8_t mt);

#endif // BH1750_H
