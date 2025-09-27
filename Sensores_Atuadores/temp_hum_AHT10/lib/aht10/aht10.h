// Adaptada por Felipe Correia das seguintes bibliotecas 
// Biblioteca bh1750 por Adafruit Industries
// https://github.com/adafruit/Adafruit_AHTX0/tree/master
// Biblioteca bh1750 por Juliano Oliveira
// https://github.com/jrfo-hwit/hlab/tree/main/firmware/c_cpp/examples/3_aht10_i2c_uart0
// Biblioteca TESTE_bh1750_BITDOGLAB por por Antonio Sergio Castro de Carvalho Junior
// https://github.com/ASCCJR/TESTE_aht10_BITDOGLAB

#ifndef AHT10_H
#define AHT10_H

#include "hardware/i2c.h"

// -----------------------------
// Endereços possíveis do AHT10
// -----------------------------
#define AHT10_ADDRESS_DEFAULT   0x38  ///< Endereço padrão do AHT10
#define AHT10_ADDRESS_ALT       0x39  ///< Endereço alternativo (alguns módulos permitem)

// -----------------------------
// Comandos do AHT10
// -----------------------------
static const uint8_t CMD_INIT[]    = {0xE1, 0x08, 0x00}; ///< Inicialização/calibração
static const uint8_t CMD_MEASURE[] = {0xAC, 0x33, 0x00}; ///< Dispara medição
static const uint8_t CMD_RESET     = 0xBA;               ///< Soft reset

// -----------------------------
// Bits de status
// -----------------------------
#define AHT10_STATUS_BUSY        0x80 ///< Bit ocupado (busy)
#define AHT10_STATUS_CALIBRATED  0x08 ///< Bit calibrado

// -----------------------------
// Estrutura de um sensor AHT10
// -----------------------------
typedef struct {
    i2c_inst_t *i2c;   ///< Instância I2C usada (ex: i2c0 ou i2c1)
    uint8_t addr;      ///< Endereço I2C do sensor
    float temperature; ///< Última leitura de temperatura (°C)
    float humidity;    ///< Última leitura de umidade (%RH)
} AHT10;

// -----------------------------
// Funções principais
// -----------------------------

/**
 * @brief Inicializa o sensor AHT10.
 * 
 * @param sensor Ponteiro para struct AHT10.
 * @param i2c    Instância do I2C (ex: i2c0).
 * @param addr   Endereço I2C (0x38 ou 0x39).
 * @return true se inicializou corretamente, false se erro.
 */
bool aht10_init(AHT10 *sensor, i2c_inst_t *i2c, uint8_t addr);

/**
 * @brief Inicialização simplificada (endereço padrão).
 */
static inline bool aht10_init_default(AHT10 *sensor, i2c_inst_t *i2c) {
    return aht10_init(sensor, i2c, AHT10_ADDRESS_DEFAULT);
}

/**
 * @brief Realiza uma medição e atualiza a struct.
 * 
 * @param sensor Ponteiro para struct AHT10.
 * @return true se leitura ok, false se erro.
 */
bool aht10_read(AHT10 *sensor);

/**
 * @brief Obtém a temperatura em °C.
 * 
 * @param sensor Ponteiro para struct AHT10.
 * @return Temperatura em °C ou -1000 em caso de erro.
 */
float aht10_get_temperature(AHT10 *sensor);

/**
 * @brief Obtém a umidade relativa em %.
 * 
 * @param sensor Ponteiro para struct AHT10.
 * @return Umidade %RH ou -1 em caso de erro.
 */
float aht10_get_humidity(AHT10 *sensor);

/**
 * @brief Calcula o ponto de orvalho (dew point).
 * 
 * @param sensor Ponteiro para struct AHT10.
 * @return Ponto de orvalho em °C ou -1000 em caso de erro.
 */
float aht10_get_dew_point(AHT10 *sensor);

#endif // AHT10_H
