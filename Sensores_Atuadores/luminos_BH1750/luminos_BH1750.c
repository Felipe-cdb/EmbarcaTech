#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "bh1750/bh1750.h"

int main() {
    stdio_init_all();

    // Inicializa I2C0 (GP4 = SDA, GP5 = SCL)
    i2c_init(i2c0, 100 * 1000);  // 100 kHz
    gpio_set_function(0, GPIO_FUNC_I2C);
    gpio_set_function(1, GPIO_FUNC_I2C);
    gpio_pull_up(0);
    gpio_pull_up(1);

    // Declara e inicializa o sensor BH1750 com endereço padrão
    BH1750 sensor;
    bh1750_init_default(&sensor, i2c0);  // Endereço padrão 0x23, modo contínuo H-RES

    // Trabalhando com um segundo sensor 
    //(deve passar o endereço alternativo ou padrão dependendo do endereço do sensor anterior)
    //BH1750 sensor2;
    //bh1750_init(&sensor2, i2c0, BH1750_ADDRESS_ALT, BH1750_ONE_TIME_H_RES_MODE);


    // Ajusta o tempo de medição (sensibilidade)
    bh1750_set_measurement_time(&sensor, 100); // aumenta sensibilidade

    while (1) {
        // Lê a luminosidade
        float lux = bh1750_read_lux(&sensor);
        if (lux < 0) {
            printf("Erro ao ler sensor BH1750!\n");
        } else {
            printf("Luminosidade: %.2f lx\n", lux);
        }

        // Demonstração: mudar temporariamente para modo one-time
        bh1750_init(&sensor, i2c0, BH1750_ADDRESS_DEFAULT, BH1750_ONE_TIME_H_RES_MODE);
        lux = bh1750_read_lux(&sensor);
        printf("Leitura One-Time H-RES: %.2f lx\n", lux);

        // Volta para modo contínuo H-RES
        bh1750_init(&sensor, i2c0, BH1750_ADDRESS_DEFAULT, BH1750_CONTINUOUSLY_H_RES_MODE);

        sleep_ms(2000); // espera 2 segundos antes da próxima leitura
    }

    return 0;
}
