#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aht10/aht10.h"

int main() {
    stdio_init_all();

    // Inicializa I2C0
    i2c_init(i2c0, 100 * 1000); // 100 kHz
    gpio_set_function(0, GPIO_FUNC_I2C); // SDA
    gpio_set_function(1, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(0);
    gpio_pull_up(1);

    // Inicializa sensor AHT10
    AHT10 sensor;
    if (!aht10_init_default(&sensor, i2c0)) {
        printf("Erro ao inicializar AHT10!\n");
        return -1;
    }

    // caso precise trabalha com outro sensor no mesmo i2c
    // Criação dos sensores
    // AHT10 sensor2;
    // //Inicialização do sensor2 com endereço alternativo
    // if (!aht10_init(&sensor2, i2c0, AHT10_ADDRESS_ALTERNATE)) {
    //     printf("Erro ao inicializar sensor2 no endereco 0x%02X\n", AHT10_ADDRESS_ALTERNATE);
    // } else {
    //     printf("Sensor2 inicializado com sucesso no endereco 0x%02X\n", AHT10_ADDRESS_ALTERNATE);
    // }

    printf("Sensor AHT10 inicializado com sucesso!\n");

    while (1) {
        float temperature = aht10_get_temperature(&sensor);
        float humidity    = aht10_get_humidity(&sensor);
        float dew_point   = aht10_get_dew_point(&sensor);

        if (temperature > -1000.0f && humidity >= 0.0f) {
            printf("Temperatura: %.2f C | Umidade: %.2f %% | Dew Point: %.2f C\n",
                   temperature, humidity, dew_point);
        } else {
            printf("Erro na leitura do sensor AHT10\n");
        }

        sleep_ms(1000); // aguarda 1 segundo
    }

    return 0;
}
