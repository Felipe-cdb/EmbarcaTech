#include <stdio.h>
#include "pico/stdlib.h"
#include "bitdoglab_sdcard/sd_card.h"

// Estrutura para guardar um conjunto de leituras de sensor.
typedef struct {
    int contador;
    float temperatura;
    int umidade;
    // Adicione mais campos aqui no futuro, se precisar!
} sensor_data_t;

// Converte um conjunto de leituras de sensor para um JSON.
void format_data_to_json(const sensor_data_t* data, char* buffer, int buffer_size) {
    snprintf(buffer, buffer_size,
        "{\n"
        "\t\t\"contador\": %d,\n"
        "\t\t\"temperatura\": %.1f,\n"
        "\t\t\"umidade\": %d\n"
        "\t}",
        data->contador,
        data->temperatura,
        data->umidade
    );
}

int main() {
    stdio_init_all();
    sleep_ms(3000); 
    printf("--- Programa Iniciado ---\n");

    printf("Inicializando SD Card...\n");
    if (!sd_card_init()) {
        printf("Não foi possível montar o SD Card.\n");
        while(true); 
    }

    int contador = 0;
    char buffer_escrita[150];
    char buffer_json[150];
    char buffer_leitura[150];

    while (true) {
        printf("\n--- Ciclo de Escrita #%d ---\n", contador);

        // --- Usando nome de ficheiro longo para o log ---
        sprintf(buffer_escrita, "Ciclo %d: Sistema OK.\n", contador);
        sd_card_write_text("system_log.txt", buffer_escrita);

        // --- Preencha a estrutura com os dados atuais ---
        sensor_data_t dados_atuais;
        dados_atuais.contador = contador;
        dados_atuais.temperatura = 25.5f + (contador * 0.1f);
        dados_atuais.umidade = 60 - contador;

        // --- Chame a função para formatar os dados ---
        format_data_to_json(&dados_atuais, buffer_json, sizeof(buffer_json));

        // --- Salve a string resultante no cartão SD ---
        sd_card_append_json("sensor_data.json", buffer_json);

        contador++;

        // -- A quantidade de contéudo que será imprimido varia confirme o tamanho do buffer de leitura --
        sd_card_read("system_log.txt", buffer_leitura, 150);
        sd_card_read("sensor_data.json", buffer_leitura, 150);

        printf("Aguardando 5 segundos para o próximo ciclo...\n");
        sleep_ms(5000);
    }

    return 0;
}