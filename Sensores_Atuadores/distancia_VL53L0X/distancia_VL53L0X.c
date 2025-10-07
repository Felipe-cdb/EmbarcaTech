// Adaptada por Felipe Correia das seguintes bibliotecas 
// Baseada na biblioteca TESTE_vl53l0x_BITDOGLAB por Antonio Sergio Castro de Carvalho Junior
// https://github.com/ASCCJR/TESTE_vl53l0x_BITDOGLAB 

#include <stdio.h>
#include "pico/stdlib.h"
#include "vl53l0x/vl53l0x.h"

// --- Configuração da Porta I2C da BitDogLab ---
// Alterne os comentários para escolher a porta desejada.
#define I2C_PORT i2c0
const uint I2C_SDA_PIN = 0;
const uint I2C_SCL_PIN = 1;

// #define I2C_PORT i2c1
// const uint I2C_SDA_PIN = 2;
// const uint I2C_SCL_PIN = 3;



int main() {
    stdio_init_all();
    // Espera ativa pela conexão do monitor serial.
    sleep_ms(100);

    printf("--- Iniciando Sensor VL53L0X (Modo Contínuo) ---\n");

    // Inicializa o sensor. É crucial verificar o retorno desta função.
    VL53L0X sensor;
    if (!vl53l0x_init(&sensor, I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN)) {
        printf("ERRO: Falha ao inicializar o sensor VL53L0X.\n");
        while (1);
    }
    printf("Sensor VL53L0X inicializado com sucesso.\n");

    // Inicia o modo de medição contínua (0ms = o mais rápido possível).
    vl53l0x_start_continuous(&sensor, 0);
    printf("Sensor em modo contínuo. Coletando dados...\n");

    while (1) {
        // Apenas lê o último resultado da medição contínua.
        // uint16_t distance = vl53l0x_read_continuous_mm(&sensor);
        
        // // Trata os possíveis valores de erro/timeout retornados pelo driver.
        // if (distance == 65535) {
        //     printf("Timeout ou erro de leitura.\n");
        // } else {
        //     // Um valor muito alto (>8000) geralmente indica que o alvo está fora do alcance do sensor.
        //     if (distance > 8000) {
        //          printf("Fora de alcance.\n");
        //     } else {
        //          printf("Distancia: %d mm\n", distance);
        //     }
        // }

        // Apenas lê o último resultado da medição contínua.
        uint16_t distance_cm = vl53l0x_read_continuous_cm(&sensor);
        
        // Trata os possíveis valores de erro/timeout retornados pelo driver.
        if (distance_cm == 6553) {
            printf("Timeout ou erro de leitura.\n");
        } else {
            // Um valor muito alto (>800) geralmente indica que o alvo está fora do alcance do sensor.
            if (distance_cm > 800) {
                 printf("Fora de alcance.\n");
            } else {
                 printf("Distancia: %d cm\n", distance_cm);
            }
        }

        // Pausa para não sobrecarregar o monitor serial.
        sleep_ms(1000); 
    }
    return 0;
}