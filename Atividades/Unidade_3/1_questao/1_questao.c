#include <stdio.h>
#include "pico/stdlib.h"
#include "bh1750/bh1750.h"
#include "fs90r/fs90r.h" // funções da biblioteca FS90R

// --- Configuração da Porta e Pinos para sensor BH1750 ---
#define I2C_PORT i2c0
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1

// Definindo pino para o servo motor FS90R
#define SERVO_PIN 2

int main()
{
    stdio_init_all(); // Inicializa a comunicação serial

    // Inicializa o sensor. É crucial verificar o retorno desta função.
    BH1750 sensorLuz;
    if (bh1750_init(&sensorLuz, I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN)){
        printf("Sensor BH1750 inicializado com sucesso.\n");
    } else {
        printf("ERRO: Falha ao inicializar o sensor BH1750.\n");
        return -1; // Finaliza o programa em caso de falha na inicialização
    }

    //Instaciando o servo
    Servo meu_servo = servo_init(SERVO_PIN);
    printf("Servo Motor inicializado com sucesso.\n");
    servo_stop(&meu_servo); // inicializa o servo parado
    sleep_ms(1000);

    // Ajusta o tempo de medição (sensibilidade)
    bh1750_set_measurement_time(&sensorLuz, 100); // Aumenta sensibilidade (tempo de medição maior)

    while (true){
        float lux = bh1750_read_lux(&sensorLuz);
        if (lux < 0) {
            printf("Erro ao ler sensor BH1750!\n");
        } else {
            printf("Luminosidade: %.2f lx\n", lux);
        }

        uint8_t velocidade;

        // Intervalo da leitura para converter a velocidade do servo
        if (lux < 0) lux = 0;
        if (lux > 2000) lux = 2000;

        if (lux < 100) {
        // Mapeamento linear entre 0 a 100 lux para a faixa de 0 a 30% de velocidade
            velocidade = (uint8_t)((lux / 100.0f) * 30);
        } else if (lux >= 100 && lux <= 2000) {
        // Mapeamento linear entre 100 a 2000 lux para a faixa de 30 a 100% de velocidade
            velocidade = 30 + (uint8_t)(((lux - 100.0f) / 1900.0f) * 70);
        }

        // Controla o servo motor com a velocidade calculada
        servo_clockwise(&meu_servo, velocidade);
        sleep_ms(3000);
    }
}
