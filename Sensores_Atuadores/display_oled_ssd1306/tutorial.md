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