#include <stdio.h>
#include "pico/stdlib.h" 
#include "aht10/aht10.h"            // funções da biblioteca aht10
#include "ssd1306/ssd1306.h"        // funções da biblioteca SSD1306
#include "hardware/i2c.h"   // i2c_init e controle do I2C
#include <string.h>         // memset
#include <ctype.h>          // toupper

// Define de portas I2C e pinos para o sensor AHT10
#define I2C_PORT_AHT10 i2c0
#define SDA_PIN_AHT10 0
#define SCL_PIN_AHT10 1

//  Define de portas I2C e pinos para o display
#define I2C_PORT_DISPLAY i2c1
#define SDA_PIN_DISPLAY 14
#define SCL_PIN_DISPLAY 15


int main() {
    stdio_init_all();

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

    memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
    char *text[] = {
        "               ",
        " Sensor AHT10  ",
        " Raspberry Pi  ",
        "   pico W      ",
        "        ______ ",
        "       |(.)(.)|",
        "       |  --  |",
        "       |      |",
    };
    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        // escreve string no buffer (x, y em pixels)
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8; // pula 8 pixels (altura da fonte)
    }
    render_on_display(ssd, &frame_area); // atualiza display
    sleep_ms(3000); // aguarda 3 segundo

    // Inicializa sensor AHT10
    AHT10 sensor_hum_temp;
    if (!aht10_init(&sensor_hum_temp, I2C_PORT_AHT10, SDA_PIN_AHT10, SCL_PIN_AHT10)) {
        printf("Erro ao inicializar AHT10!\n");
        return -1;
    }
    
    sleep_ms(1000); // aguarda 1s para estabilização do sensor
    printf("Sensor AHT10 inicializado com sucesso!\n");

    while (true) {
        float temperatura = aht10_get_temperature(&sensor_hum_temp);
        float umidade    = aht10_get_humidity(&sensor_hum_temp);

        // verifica se a leitura foi bem sucedida, caso contrário finaliza o programa
        if (temperatura <= -1000.0f || umidade <= -1) {
            printf("Erro na leitura do sensor AHT10\n");
            return -1;
        } 

        // Alerta de temperatura baixa
        if (temperatura < 20.0f) {
            printf("Alerta: Temperatura baixa! (%.2f C)\n", temperatura);
            memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
            // Buffers de texto dinâmico
            char linha1[32], linha2[32], linha3[32];

            sprintf(linha1, " !! Alerta !!  ");
            sprintf(linha2, "   Temperatura ");
            // escapes hex \xBA para caractere º
            sprintf(linha3, "      Baixa    ");

            // Array de ponteiros para as strings
            char *text[] = {
                linha1,
                linha2,
                linha3,
                "!             !",
                "!             !",
                "!             !",
                "!             !",
                "----!!!!!!!----",
            };
            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                // escreve string no buffer (x, y em pixels)
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 8; // pula 8 pixels (altura da fonte)
            }
            render_on_display(ssd, &frame_area); // atualiza display
            sleep_ms(3000); // aguarda 3 segundo
        }

        // Alerta de umidade alta
        if (umidade > 70.0f) {
            printf("Alerta: Umidade alta! (%.2f %%)\n", umidade);

            memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
            // Buffers de texto dinâmico
            char linha1[32], linha2[32], linha3[32];

            sprintf(linha1, "  !! Alerta !! ");
            sprintf(linha2, "     Umidade   ");
            // escapes hex \xBA para caractere º
            sprintf(linha3, "      Alta     ");

            // Array de ponteiros para as strings
            char *text[] = {
                linha1,
                linha2,
                linha3,
                "!             !",
                "!             !",
                "!             !",
                "!             !",
                "----!!!!!!!----",
            };
            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                // escreve string no buffer (x, y em pixels)
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 8; // pula 8 pixels (altura da fonte)
            }
            render_on_display(ssd, &frame_area); // atualiza display
            sleep_ms(3000); // aguarda 3 segundo
        }

        printf("Temperatura: %.2f C | Umidade: %.2f %%\n",temperatura, umidade);

        memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
        // Buffers de texto dinâmico
        char linha1[32], linha2[32], linha3[32], linha4[32];

        sprintf(linha1, " Temperatura:");
        sprintf(linha2, " %.2f *C", temperatura);
        sprintf(linha3, " Umidade:");
        sprintf(linha4, " %.2f %%", umidade);

        // Array de ponteiros para as strings
        char *text[] = {
            linha1,
            linha2,
            linha3,
            linha4,
            "        ______ ",
            "       |(.)(.)|",
            "       |  --  |",
            "       |      |",
        };
        int y = 0;
        for (uint i = 0; i < count_of(text); i++) {
            // escreve string no buffer (x, y em pixels)
            ssd1306_draw_string(ssd, 5, y, text[i]);
            y += 8; // pula 8 pixels (altura da fonte)
        }
        render_on_display(ssd, &frame_area); // atualiza display
        sleep_ms(3000); // aguarda 3 segundo
    }
}
