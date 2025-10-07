#include <stdio.h>
#include "pico/stdlib.h"
#include "vl53l0x/vl53l0x.h"
#include "bitdoglab_sdcard/sd_card.h"
#include "ssd1306/ssd1306.h"// funções da biblioteca SSD1306
#include "hardware/i2c.h"   // i2c_init e controle do I2C
#include <string.h>         // memset
#include <ctype.h>          // toupper

// --- Configuração da Porta I2C da BitDogLab ---
// Alterne os comentários para escolher a porta desejada.
#define I2C_PORT i2c0
const uint I2C_SDA_PIN = 0;
const uint I2C_SCL_PIN = 1;

//  Define de portas I2C e pinos para o display
#define I2C_PORT_DISPLAY i2c1
#define SDA_PIN_DISPLAY 14
#define SCL_PIN_DISPLAY 15

const uint16_t contador = 0; 

int main()
{
    stdio_init_all();
    // Espera ativa pela conexão do monitor serial.
    sleep_ms(100);

    // Inicialização do I2C para display
    i2c_init(I2C_PORT_DISPLAY, ssd1306_i2c_clock * 1000); 
    gpio_set_function(SDA_PIN_DISPLAY, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_DISPLAY, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_DISPLAY);
    gpio_pull_up(SCL_PIN_DISPLAY);

    // Inicialização do display
    ssd1306_init();

    // Área de renderização: define qual parte da tela será atualizada
    struct render_area frame_area = {
        .start_column = 0,
        .end_column   = ssd1306_width - 1,
        .start_page   = 0,
        .end_page     = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&frame_area);

    // Buffer local (RAM): aqui desenhamos antes de enviar ao display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);   // limpa o buffer
    render_on_display(ssd, &frame_area);     // envia tela limpa

    printf("Inicializando SD Card...\n");
    if (!sd_card_init()) {
        printf("Não foi possível montar o SD Card.\n");
        while(true); 
    }
    
    int contador = 0;
    char buffer_escrita[150];
    char buffer_json[150];
    char buffer_leitura[150];


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

    while (true) {

        // ---------------------------------------------------------------
        // --- Lê a distância em centímetros. ----------------------------
        // ---------------------------------------------------------------
        uint16_t distance_cm = vl53l0x_read_continuous_cm(&sensor);
        // Trata os possíveis valores de erro/timeout retornados pelo driver.
        if (distance_cm == 6553) {
            printf("Timeout ou erro de leitura.\n");
            return -1; // Encerra o programa em caso de erro.
        }

        // Um valor muito alto (>800) geralmente indica que o alvo está fora do alcance do sensor.
        if (distance_cm > 800) {
            printf("Fora de alcance.\n");
            distance_cm = -1;
        } else {
            printf("Distancia: %d cm\n", distance_cm);
        }




        // ---------------------------------------------------------------
        // --- Imprime no display ----------------------------------------
        // ---------------------------------------------------------------
        memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
        // Buffers de texto dinâmico
        char linha1[32], linha2[32];

        sprintf(linha1, " Distancia:");
        sprintf(linha2, " %d cm", distance_cm);

        // Array de ponteiros para as strings
        char *text[] = {
            linha1,
            linha2,
            "        ______ ",
            "       | _  _ |",
            "       |(.)(.)|",
            "       |  __  |",
            "       |      |",
            "       |      |",
        };
        int y = 0;
        for (uint i = 0; i < count_of(text); i++) {
            // escreve string no buffer (x, y em pixels)
            ssd1306_draw_string(ssd, 5, y, text[i]);
            y += 8; // pula 8 pixels (altura da fonte)
        }
        render_on_display(ssd, &frame_area); // atualiza display



        // ---------------------------------------------------------------
        // --- Formatação e gravação de dados no cartão SD ---------------
        // ---------------------------------------------------------------

        sprintf(buffer_escrita, "Ciclo %d: Distancia: %d cm.\n", contador, distance_cm);
        sd_card_write_text("system_log.txt", buffer_escrita);

        contador = contador + 1;

        // -- A quantidade de contéudo que será imprimido varia confirme o tamanho do buffer de leitura --
        sd_card_read("system_log.txt", buffer_leitura, 150);


        // Aguarda 3 segundos antes da próxima leitura.
        sleep_ms(3000); 
    }
    return 0;
}
