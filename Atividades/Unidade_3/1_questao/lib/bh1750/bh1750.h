// Adaptada por Felipe Correia das seguintes bibliotecas 
// Biblioteca bh1750 por dernasherbrezon
// https://github.com/dernasherbrezon/bh1750
// Biblioteca TESTE_bh1750_BITDOGLAB por por Antonio Sergio Castro de Carvalho Junior
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
    i2c_inst_t *i2c;        // Instância do barramento I2C
    uint8_t sda, scl;       // Pinos SDA e SCL do I2C
    uint8_t addr;           // Endereço I2C do sensor
    uint8_t mode;           // Modo de operação (contínuo ou one-time)
} BH1750;

// Função de inicialização com parâmetros padrão
bool bh1750_init(BH1750* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl);
// Função de inicialização customizada
bool bh1750_init_custom(BH1750* sensor, i2c_inst_t* i2c, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t mode);

// Lê o valor de luminosidade em lux
float bh1750_read_lux(BH1750 *sensor);

// Função para alterar o modo do sensor (contínuo ou one-time)
void bh1750_set_mode(BH1750* sensor, uint8_t mode);

// Configura o tempo de medição (31 a 254). Padrão é 69.
// Valores menores aumentam a sensibilidade (mais lux), mas tornam a medição mais lenta
void bh1750_set_measurement_time(BH1750 *sensor, uint8_t mt);

#endif // BH1750_H
