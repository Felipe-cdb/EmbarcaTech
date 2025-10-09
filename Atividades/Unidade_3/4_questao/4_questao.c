#include <stdio.h>
#include "pico/stdlib.h"
#include "fs90r/fs90r.h"
#include "mpu6050/mpu6050.h"
#include "ssd1306/ssd1306.h"        // funções da biblioteca SSD1306
#include "hardware/i2c.h"   // i2c_init e controle do I2C
#include <string.h>         // memset
#include <ctype.h>          // toupper

// Definindo pino para o servo motor FS90R
#define SERVO_PIN 2

// Definindo pinos para acelerômetro MPU6050
#define I2C_SDA_PIN 0
#define I2C_SCL_PIN 1
#define I2C_PORT i2c0

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

    MPU6050 sensor_movimento;
    if (!mpu6050_init(&sensor_movimento, I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN)) {
        return -1; // Falha na inicialização do sensor
    }
    
    //Instaciando o servo
    Servo meu_servo = servo_init(SERVO_PIN);
    servo_stop(&meu_servo); // inicializa o servo parado
    sleep_ms(1000);

    while (true) {
        // Lê os dados crus (sem conversão)
        if (mpu6050_read_raw(&sensor_movimento)) {
            if (sensor_movimento.accel_raw[0] > 0){
                // movimento para a direita
                printf("DIR: X=%d \n", sensor_movimento.accel_raw[0]);
                
                servo_stop(&meu_servo);
                sleep_ms(500);
                servo_clockwise(&meu_servo, 70);


                // ---------------------------------------------------------------
                // --- Imprime no display ----------------------------------------
                // ---------------------------------------------------------------
                memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
                // Buffers de texto dinâmico
                char linha1[32], linha2[32];

                sprintf(linha1, " Direita:");
                sprintf(linha2, "x = %d", sensor_movimento.accel_raw[0]);

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
            } else {
                // movimento para a esquerda
                printf("ESQ: X=%d \n", sensor_movimento.accel_raw[0]);

                servo_stop(&meu_servo);
                sleep_ms(500);
                servo_counterclockwise(&meu_servo, 70);


                // ---------------------------------------------------------------
                // --- Imprime no display ----------------------------------------
                // ---------------------------------------------------------------
                memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
                // Buffers de texto dinâmico
                char linha1[32], linha2[32];

                sprintf(linha1, " Esquerda:");
                sprintf(linha2, "x = %d", sensor_movimento.accel_raw[0]);

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
            }

        } else {
            printf("Erro na leitura crua do MPU6050.\n");
            return -1;
        }
        sleep_ms(1000);
    }
}
