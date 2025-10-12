#include <stdio.h>
#include "pico/stdlib.h"
#include "aht10/aht10.h"                // funções da biblioteca aht10
#include "vl53l0x/vl53l0x.h"            // funções da biblioteca vl53l0x
#include "bitdoglab_sdcard/sd_card.h"   // funções da biblioteca sd_card

// Define de portas I2C e pinos para o sensor AHT10
#define I2C_PORT_AHT10 i2c0
#define SDA_PIN_AHT10 0
#define SCL_PIN_AHT10 1

// Define de portas I2C e pinos para o sensor VL53L0X
#define I2C_PORT_DIST i2c1
#define SDA_PIN_DIST 2
#define SCL_PIN_DIST 3

// Função para converter milissegundos desde o boot para horas, minutos, segundos e data
void ms_to_date_time(uint64_t ms, int *day, int *hour, int *minute, int *second) {
    // Defina a data e hora inicial (data de boot)
    // Por exemplo, definimos um ponto de referência (dia 1, hora 0)
    int base_day = 1;     // Data inicial (ex: dia 1)
    int base_hour = 0;    // Hora inicial
    int base_minute = 0;  // Minuto inicial
    int base_second = 0;  // Segundo inicial

    // Cálculo do tempo a partir da data inicial
    *second = (ms / 1000) % 60;
    *minute = (ms / 60000) % 60;
    *hour = (ms / 3600000) % 24;
    *day = base_day + (ms / 86400000);  // 86400000 ms por dia

    // Ajuste para o tempo de boot, se necessário (essa parte é um exemplo)
    *second += base_second;
    *minute += base_minute;
    *hour += base_hour;
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // ------------------------------------------------------------------
    // -------------------Inicializa sensor AHT10------------------------
    // ------------------------------------------------------------------
    AHT10 sensor_hum_temp;
    if (!aht10_init(&sensor_hum_temp, I2C_PORT_AHT10, SDA_PIN_AHT10, SCL_PIN_AHT10)) {
        printf("Erro ao inicializar AHT10!\n");
        return -1; // finaliza o programa com erro
    }
   
    printf("Sensor AHT10 inicializado com sucesso!\n");
    sleep_ms(1000); // aguarda 1s para estabilização do sensor



    // ------------------------------------------------------------------
    // -------------------Inicializa sdcard------------------------------
    // ------------------------------------------------------------------
    if (!sd_card_init()) {
        printf("Não foi possível montar o SD Card.\n");
        return -1; // finaliza o programa com erro
    }
   
    char buffer_escrita[150];
    // char buffer_leitura[150];
    printf("SD Card inicializado com sucesso!\n");
    sleep_ms(1000);

    
    // ------------------------------------------------------------------
    // -------------------Inicializa sensor VL53L0X----------------------
    // ------------------------------------------------------------------
    VL53L0X sensorDist;
    if (!vl53l0x_init(&sensorDist, I2C_PORT_DIST, SDA_PIN_DIST, SCL_PIN_DIST)) {
        printf("ERRO: Falha ao inicializar o sensor VL53L0X.\n");
        return -1; // finaliza o programa com erro
    }
    printf("Sensor VL53L0X inicializado com sucesso.\n");
    sleep_ms(1000);

    // Inicia o modo de medição contínua (0ms = o mais rápido possível).
    vl53l0x_start_continuous(&sensorDist, 0);
    printf("Sensor em modo contínuo. Coletando dados...\n");

    while (true) {

        float temperatura = aht10_get_temperature(&sensor_hum_temp);
        float umidade    = aht10_get_humidity(&sensor_hum_temp);
        uint16_t distance_cm = vl53l0x_read_continuous_cm(&sensorDist);
        uint64_t timestamp_ms = to_ms_since_boot(get_absolute_time());

// Converte os milissegundos desde o boot para hora, minuto, segundo
        int day, hour, minute, second;
        ms_to_date_time(timestamp_ms, &day, &hour, &minute, &second);

        // verifica se a leitura de tem_hum foi bem sucedida
        if (temperatura <= -1000.0f || umidade <= -1) {
            printf("Erro na leitura do sensor AHT10\n");
            return -1; // Encerra o programa em caso de erro.
        }

        // Trata os possíveis valores de erro/timeout retornados pelo driver.
        if (distance_cm == 6553) {
            printf("Timeout ou erro de leitura.\n");
            return -1; // Encerra o programa em caso de erro.
        }

        if (distance_cm <= 30){
            printf("Detecção de proximidade\n");
            
            // Registra o evento de proximidade no arquivo SD com timestamp
            sprintf(buffer_escrita, "Detecção de proximidade as %02d:%02d:%02d | Dia: %d\n", hour, minute, second, day);
            sd_card_write_text("Datalogger.txt", buffer_escrita);
        }
        
        // Exibe os valores lidos no console com timestamp
        printf("Dia: %d | Hora: %02d:%02d:%02d | Temperatura: %.2f C | Umidade: %.2f %%\n", day, hour, minute, second, temperatura, umidade);

        // Registra os dados no arquivo SD com timestamp
        sprintf(buffer_escrita, "Dia: %d | Hora: %02d:%02d:%02d | Temperatura: %.2f C | Umidade: %.2f %%\n", day, hour, minute, second, temperatura, umidade);
        sd_card_write_text("Datalogger.txt", buffer_escrita);

        sleep_ms(5000);
    }
}
