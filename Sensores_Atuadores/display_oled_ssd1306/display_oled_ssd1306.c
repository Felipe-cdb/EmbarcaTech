#include "pico/stdlib.h"    // sleep_ms, stdio_init_all, GPIO
#include "hardware/i2c.h"   // i2c_init e controle do I2C
#include "ssd1306/ssd1306.h"        // funções da biblioteca SSD1306
#include <string.h>         // memset
#include <ctype.h>          // toupper
#include "logo_embarcatech.h" // bitmap de exemplo

// Definição dos pinos I2C1 usados para o display
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int main() {
    stdio_init_all();   // Inicializa stdio (USB/UART). Opcional.

    // --------------------------
    // 1) Inicialização do I2C
    // --------------------------
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);   // Clock definido em ssd1306_i2c.h
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // --------------------------
    // 2) Inicialização do display
    // --------------------------
    ssd1306_init();   // Envia os comandos básicos para o SSD1306

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

    // --------------------------
    // 3) Texto no display
    // --------------------------
    char *text[] = {
        " Bem-vindos!   ",
        " OLED SSD1306  ",
        " Raspberry Pi  ",
        " [{pico W}]    ",
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
    sleep_ms(5000);

    // --------------------------
    // 4) Desenho de linha
    // --------------------------
    memset(ssd, 0, ssd1306_buffer_length);   // limpa buffer
    ssd1306_draw_line(ssd, 0, 0, 127, 63, true); // diagonal
    render_on_display(ssd, &frame_area);
    sleep_ms(2000);

    // --------------------------
    // 5) Pixels individuais
    // --------------------------
    memset(ssd, 0, ssd1306_buffer_length);
    for (int x = 0; x < 128; x += 4) {
        ssd1306_set_pixel(ssd, x, 32, true); // linha de pontos no meio
    }
    render_on_display(ssd, &frame_area);
    sleep_ms(1000);

    // --------------------------
    // 6) Scroll automático
    // --------------------------
    ssd1306_scroll(true);  // ativa scroll de hardware
    sleep_ms(500);
    ssd1306_scroll(false); // desativa scroll

    // --------------------------
    // 7) Bitmap
    // --------------------------
    // Exemplo simplificado (logo ou ícone)
    // Obs: bitmap deve ter tamanho width*height/8 bytes
    ssd1306_t ssd_bm;
    ssd1306_init_bm(&ssd_bm, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&ssd_bm);

    ssd1306_draw_bitmap(&ssd_bm, logo_embarcatech);

    while (true) {
        sleep_ms(1000);
    }

    return 0;
}
