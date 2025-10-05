#include <stdio.h>
#include "pico/stdlib.h"
#include "bh1750/bh1750.h"

// --- Configuração da Porta I2C da BitDogLab ---
// Alterne os comentários para escolher a porta desejada.
#define I2C_PORT i2c0
#define I2C_SDA_PIN 0   // Corrigido o uso do "=" para a atribuição correta
#define I2C_SCL_PIN 1   // Corrigido o uso do "=" para a atribuição correta

int main() {
    stdio_init_all();

    // Inicializa o sensor. É crucial verificar o retorno desta função.
    BH1750 sensor;
    if (bh1750_init(&sensor, I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN)) {
        printf("Sensor BH1750 inicializado com sucesso.\n");
    } else {
        printf("ERRO: Falha ao inicializar o sensor BH1750.\n");
        return -1;  // Finaliza o programa em caso de falha na inicialização
    }

    // Ajusta o tempo de medição (sensibilidade)
    bh1750_set_measurement_time(&sensor, 100); // Aumenta sensibilidade (tempo de medição maior)

    while (1) {
        // Lê a luminosidade no modo contínuo
        float lux = bh1750_read_lux(&sensor);
        if (lux < 0) {
            printf("Erro ao ler sensor BH1750!\n");
        } else {
            printf("Luminosidade: %.2f lx\n", lux);
        }

        // Demonstração: mudar temporariamente para modo one-time
        bh1750_set_mode(&sensor, BH1750_ONE_TIME_H_RES_MODE);
        lux = bh1750_read_lux(&sensor);
        printf("Leitura One-Time H-RES: %.2f lx\n", lux);

        sleep_ms(5000); // Espera 5 segundos antes da próxima leitura
        
        // Volta para modo contínuo H-RES
        bh1750_set_mode(&sensor, BH1750_CONTINUOUSLY_H_RES_MODE);
        for (size_t i = 0; i < 10; i++)
        {
            lux = bh1750_read_lux(&sensor);
            printf("Leitura Continua H-RES: %.2f lx\n", lux);
            sleep_ms(2000); // Espera 2 segundos antes da próxima leitura
        }

        sleep_ms(10000); // Espera 10 segundos antes da próxima leitura
    }

    return 0;
}
