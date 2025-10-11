// Adaptada por Felipe Correia das seguintes bibliotecas 
// Baseada na biblioteca TESTE_vl53l0x_BITDOGLAB por Antonio Sergio Castro de Carvalho Junior
// https://github.com/ASCCJR/TESTE_vl53l0x_BITDOGLAB 

#ifndef VL53L0X_H
#define VL53L0X_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Endereço padrão do VL53L0X
#define VL53L0X_ADDRESS 0x29

// Endereços alternativos do VL53L0X, para diferentes configurações de hardware
#define VL53L0X_ADDRESS_ALTERNATE 0x2A
#define VL53L0X_ADDRESS_ALTERNATE2 0x2B
#define VL53L0X_ADDRESS_ALTERNATE3 0x2C
#define VL53L0X_ADDRESS_ALTERNATE4 0x2D

// Estrutura do sensor VL53L0X
typedef struct {
    i2c_inst_t* i2c;               // Instância do barramento I2C
    uint8_t sda, scl;              // Pinos SDA e SCL do I2C
    uint8_t address;               // Endereço I2C do sensor
    uint16_t io_timeout;           // Tempo máximo de espera para operação (em ms)
    uint8_t stop_variable;         // Variável usada para controle interno do sensor
    uint32_t measurement_timing_budget_us; // Orçamento de tempo para medições (em microsegundos)
} VL53L0X;

/**
 * @brief Enumeração completa dos registradores do VL53L0X.
 * Usar nomes em vez de números hexadecimais torna o código do driver mais legível.
 */
enum regAddr {
  SYSRANGE_START                              = 0x00, // Início da medição
  SYSTEM_THRESH_HIGH                          = 0x0C, // Limite superior de faixa
  SYSTEM_THRESH_LOW                           = 0x0E, // Limite inferior de faixa
  SYSTEM_SEQUENCE_CONFIG                      = 0x01, // Configuração da sequência de medição
  SYSTEM_RANGE_CONFIG                         = 0x09, // Configuração do modo de medição
  SYSTEM_INTERMEASUREMENT_PERIOD              = 0x04, // Período entre medições
  SYSTEM_INTERRUPT_CONFIG_GPIO                = 0x0A, // Configuração de interrupção GPIO
  GPIO_HV_MUX_ACTIVE_HIGH                     = 0x84, // Controle do multiplexador HV
  SYSTEM_INTERRUPT_CLEAR                      = 0x0B, // Limpeza da interrupção
  RESULT_INTERRUPT_STATUS                     = 0x13, // Status da interrupção
  RESULT_RANGE_STATUS                         = 0x14, // Status da medição
  RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN       = 0xBC, // Eventos de janela de ambiente
  RESULT_CORE_RANGING_TOTAL_EVENTS_RTN        = 0xC0, // Eventos totais de medição
  RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF       = 0xD0, // Referência de eventos de janela de ambiente
  RESULT_CORE_RANGING_TOTAL_EVENTS_REF        = 0xD4, // Referência de eventos totais de medição
  RESULT_PEAK_SIGNAL_RATE_REF                 = 0xB6, // Taxa de pico de sinal de referência
  ALGO_PART_TO_PART_RANGE_OFFSET_MM           = 0x28, // Deslocamento de parte a parte
  I2C_SLAVE_DEVICE_ADDRESS                    = 0x8A, // Endereço do dispositivo I2C escravo
  MSRC_CONFIG_CONTROL                         = 0x60, // Controle de configuração MSRC
  PRE_RANGE_CONFIG_MIN_SNR                    = 0x27, // SNR mínimo de pré-faixa
  PRE_RANGE_CONFIG_VALID_PHASE_LOW            = 0x56, // Fase válida baixa de pré-faixa
  PRE_RANGE_CONFIG_VALID_PHASE_HIGH           = 0x57, // Fase válida alta de pré-faixa
  PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          = 0x64, // Limite mínimo de taxa de contagem de retorno da pré-faixa
  FINAL_RANGE_CONFIG_MIN_SNR                  = 0x67, // SNR mínimo da faixa final
  FINAL_RANGE_CONFIG_VALID_PHASE_LOW          = 0x47, // Fase válida baixa da faixa final
  FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         = 0x48, // Fase válida alta da faixa final
  FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44, // Limite mínimo de taxa de contagem de retorno da faixa final
  PRE_RANGE_CONFIG_SIGMA_THRESH_HI            = 0x61, // Limite superior do sigma da pré-faixa
  PRE_RANGE_CONFIG_SIGMA_THRESH_LO            = 0x62, // Limite inferior do sigma da pré-faixa
  PRE_RANGE_CONFIG_VCSEL_PERIOD               = 0x50, // Período VCSEL da pré-faixa
  PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          = 0x51, // Timeout do macro de pré-faixa (alta)
  PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO          = 0x52, // Timeout do macro de pré-faixa (baixa)
  SYSTEM_HISTOGRAM_BIN                        = 0x81, // Histograma da medição
  HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT       = 0x33, // Configuração inicial da fase do histograma
  HISTOGRAM_CONFIG_READOUT_CTRL               = 0x55, // Controle de leitura do histograma
  FINAL_RANGE_CONFIG_VCSEL_PERIOD             = 0x70, // Período VCSEL da faixa final
  FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        = 0x71, // Timeout do macro da faixa final (alta)
  FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO        = 0x72, // Timeout do macro da faixa final (baixa)
  CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       = 0x20, // Taxa de pico de compensação de crosstalk
  MSRC_CONFIG_TIMEOUT_MACROP                  = 0x46, // Timeout do MSRC
  SOFT_RESET_GO2_SOFT_RESET_N                 = 0xBF, // Reset do sensor
  IDENTIFICATION_MODEL_ID                     = 0xC0, // ID do modelo do sensor
  IDENTIFICATION_REVISION_ID                  = 0xC2, // ID de revisão do sensor
  OSC_CALIBRATE_VAL                           = 0xF8, // Valor de calibração do oscilador
  GLOBAL_CONFIG_VCSEL_WIDTH                   = 0x32, // Largura do VCSEL
  POWER_MANAGEMENT_GO1_POWER_FORCE            = 0x80, // Força de gerenciamento de energia
  VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV           = 0x89, // Configuração de suporte HV
  ALGO_PHASECAL_LIM                           = 0x30, // Limite da calibração de fase
  ALGO_PHASECAL_CONFIG_TIMEOUT                = 0x30, // Timeout da calibração de fase
  RESULT_RANGE_MM                             = 0x1E, // Resultado da medição em mm
};

// Funções públicas para inicialização e leitura do sensor

// Inicializa o sensor com pinos I2C padrão
bool vl53l0x_init(VL53L0X* sensor, i2c_inst_t* i2c_port, uint8_t sda, uint8_t scl);
// Inicializa o sensor com endereço I2C customizado
bool vl53l0x_init_custom(VL53L0X* sensor, i2c_inst_t* i2c_port, uint8_t sda, uint8_t scl, uint8_t address);

// Inicia medição contínua com intervalo definido(Retorna 65535 em caso de erro mm, e 6553 em cm)
void vl53l0x_start_continuous(VL53L0X* sensor, uint32_t period_ms);

// Lê distâncias em milímetros
uint16_t vl53l0x_read_single_mm(VL53L0X* sensor); // Lê distância única em milímetros
uint16_t vl53l0x_read_continuous_mm(VL53L0X* sensor); // Lê distância contínua em milímetros

// Lê distâncias em centímetros
uint16_t vl53l0x_read_single_cm(VL53L0X* sensor);// Lê distância única em centímetros
uint16_t vl53l0x_read_continuous_cm(VL53L0X* sensor);// Lê distância contínua em centímetros

#endif