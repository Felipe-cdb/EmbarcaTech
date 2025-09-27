Baseada na biblioteca utilizada em
https://github.com/BitDogLab/BitDogLab-C/tree/main/ssd1306

📂 Estrutura da biblioteca

# 1 - ssd1306.h 
É o header principal. Ele declara as funções públicas que você vai usar no seu main.c (ex: ssd1306_init, ssd1306_draw_string, ssd1306_set_pixel, etc).
👉 É como o “menu” da biblioteca.

# 2 - ssd1306_font.h
Guarda os dados da fonte (tabela de caracteres).
Cada caractere é representado como um conjunto de bytes, que o display entende como pixels acesos/apagados.
👉 Sem ele, não teria como escrever texto na tela.

# 3 - ssd1306_i2c.c
Contém a implementação das funções declaradas em ssd1306.h.
Aqui estão:

Inicialização do display (ssd1306_init)

Envio de comandos/dados via I2C (ssd1306_send_command, ssd1306_send_buffer)

Desenho de pixels, linhas, caracteres e strings

Funções extras como scroll, bitmaps, etc.
👉 É o “cérebro” que fala com o display.

# 4 - ssd1306_i2c.h
Contém definições de registradores e comandos do SSD1306 (endereços de memória, instruções como ssd1306_set_display, ssd1306_set_contrast, etc).
👉 É como um “dicionário” para o ssd1306_i2c.c.

# CMakeList
Precisei do seguites configurações em meu cmakerlist: 
    # Add the standard library to the build
    target_link_libraries(display_oled_ssd1306
            pico_stdlib)

    # Add the standard include files to the build
    target_include_directories(display_oled_ssd1306 PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/lib
    )

    # Adicionar os arquivos da biblioteca ao projeto
    target_sources(display_oled_ssd1306 PRIVATE
            lib/ssd1306/ssd1306_i2c.c
    )

    # Add any user requested libraries
    target_link_libraries(display_oled_ssd1306 
            hardware_i2c
    )

# Funções disponiveis na biblioteca
ssd1306_init → inicializa o display.

render_on_display → envia o buffer local para o OLED.

ssd1306_draw_string → escreve texto.

ssd1306_draw_line → desenha linha (Bresenham).

ssd1306_set_pixel → acende/apaga pixels individuais.

ssd1306_scroll → ativa/desativa scroll automático.

ssd1306_draw_bitmap → exibe imagens pré-carregadas.

👉 Fluxo: você sempre desenha no buffer RAM (ssd[]) → depois envia para a tela (render_on_display).

# Logo/img para bitmap
passo a passo para converter qualquer imagem (como a logo da Embarcatech) para usar no SSD1306 128x64.

🖼️ Como converter imagem para bitmap do SSD1306
1. Redimensionar a imagem

O display tem 128x64 pixels.

Abra sua imagem em um editor (GIMP, Photoshop, Paint.NET, até mesmo sites online).
exemplo online(https://javl.github.io/image2cpp/)

Redimensione para 128x64 px.

Converta para preto e branco (monocromático) → nada de tons de cinza, só preto ou branco.

💡 Dica: no GIMP → Imagem > Modo > Tons de Cinza → depois Imagem > Modo > Bitmap.

2. Converter para array de bytes

Existem várias formas, mas duas comuns são:
Opção A: LCD Assistant (Windows)
Opção B: Python script (multiplataforma)

3. Usar no seu código

No main.c:

const uint8_t logo_embarcatech[] = {
  0xFF, 0xFF, 0xBF, 0xF7, 0xFF, 0xFB, 0xBF, 0xFF,
  0xF7, 0xFF, 0xBB, 0xFF, 0xDF, 0xFF, 0xFB, 0xBF,
  0xFF, 0xF7, 0xFF, 0xBB, 0xFF, 0xFF, 0x77, 0xFF,
  0xBB, 0xFF, 0xFF, 0x77, 0xFF, 0xBB, 0xFF, 0xFF,
  0x77, 0xFF, 0xBB, 0xFF, 0xDF, 0xFF, 0xFB, 0x7F,
  0xEF, 0xFF, 0x7B, 0xFF, 0xDF, 0xFF, 0x7B, 0xFF,
  0xEF, 0xFF, 0xBB, 0xFF, 0xDF, 0xFF, 0xFB, 0x7F,
  0xEF, 0xFF, 0x7B, 0xFF, 0xDF, 0xFF, 0x7B, 0xFF,
  // ... (continua até 1024 bytes no total para 128x64)
};

// Inicialização especial para bitmaps
ssd1306_t ssd_bm;
ssd1306_init_bm(&ssd_bm, 128, 64, false, 0x3C, i2c1);
ssd1306_config(&ssd_bm);

// Exibir logo
ssd1306_draw_bitmap(&ssd_bm, logo_embarcatech);