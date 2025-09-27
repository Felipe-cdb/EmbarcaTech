#include "main.h"

static uint32_t last_debounce_time[3] = {0, 0, 0};
uint brightness = 0;
bool increasing = true;

static const char *gpio_irq_str[] = {
    "LEVEL_LOW",  // 0x1
    "LEVEL_HIGH", // 0x2
    "EDGE_FALL",  // 0x4
    "EDGE_RISE"   // 0x8
};

void pinos_start()
{
    gpio_init(LED_PIN_R);
    gpio_init(LED_PIN_B);
    gpio_init(LED_PIN_G);
    adc_init();
    gpio_set_function(LED_PIN_R, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN_G, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN_B, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(LED_PIN_R);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(slice_num, &config, true);
    slice_num = pwm_gpio_to_slice_num(LED_PIN_B);
    pwm_init(slice_num, &config, true);
    slice_num = pwm_gpio_to_slice_num(LED_PIN_G);
    pwm_init(slice_num, &config, true);

    gpio_init(BUTTON6_PIN);
    gpio_set_dir(BUTTON6_PIN, GPIO_IN);
    gpio_pull_up(BUTTON6_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON6_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio5_callback);

    gpio_init(BUTTON5_PIN);
    gpio_set_dir(BUTTON5_PIN, GPIO_IN);
    gpio_pull_up(BUTTON5_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON5_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio5_callback);

    // PWM do Buzzer
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_buzzer = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config_buzzer = pwm_get_default_config();
    pwm_config_set_clkdiv(&config_buzzer, 7.8f); // Aproximadamente 1kHz
    pwm_init(slice_buzzer, &config_buzzer, true);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Começa desligado

    // OLED SSD1306
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Inicializa o display SSD1306
    ssd1306_init();

    // Limpa a tela (opcional)
    uint8_t clear_buffer[ssd1306_buffer_length] = {0};
    struct render_area area = {0, ssd1306_width - 1, 0, ssd1306_n_pages - 1, ssd1306_buffer_length};
    render_on_display(clear_buffer, &area); // Atualiza a tela para limpar

    // Exibe a mensagem "WELCOME"
    ssd1306_draw_string(clear_buffer, 0, 0, "....WELCOME....");

    // Atualiza o display para mostrar a mensagem
    render_on_display(clear_buffer, &area);

    
    // I2C Sensor distancia vl5310x. usando 400Khz.s
    i2c_init(i2c0, 400*1000);
    
    gpio_set_function(I2C_SDA_DISTANCE, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISTANCE, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISTANCE);
    gpio_pull_up(I2C_SCL_DISTANCE);
}

void gpio_event_string(char *buf, uint32_t events)
{
    for (uint i = 0; i < 4; i++)
    {
        uint mask = (1 << i);
        if (events & mask)
        {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0')
            {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events add ", "
            if (events)
            {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}

void gpio5_callback(uint gpio, uint32_t events)
{
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == 5 && (now - last_debounce_time[0] > DEBOUNCE_DELAY_MS))
    {
        last_debounce_time[0] = now;
    }
    if (gpio == 6 && (now - last_debounce_time[1] > DEBOUNCE_DELAY_MS))
    {
        last_debounce_time[1] = now;
        alarme = !alarme;
    }
}

void setup_pwm(uint gpio_pin)
{
    // Configurar o GPIO como saída de PWM
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);

    // Obter o slice de PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(gpio_pin);

    // Configurar o PWM com o padrão
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajustar divisor de clock (frequência do PWM)
    pwm_init(slice_num, &config, true);
}

void update_pwm(uint gpio_pin)
{
    // Atualizar o valor do brilho
    if (increasing)
    {
        brightness = brightness + 400;
        if (brightness >= PWM_STEPS)
        {
            increasing = false; // Começar a diminuir
        }
    }
    else
    {
        brightness = brightness - 400;
        if (brightness == 0)
        {
            increasing = true; // Começar a aumentar
        }
    }
}

void pwm_led(uint gpio_pin, uint brilho)
{
    if (gpio_pin == LED_PIN_B)
    {
        pwm_set_gpio_level(LED_PIN_B, brilho);
    }
    else if (gpio_pin == LED_PIN_G)
    {
        pwm_set_gpio_level(LED_PIN_G, brilho);
    }
    else if (gpio_pin == LED_PIN_R)
    {
        pwm_set_gpio_level(LED_PIN_R, brilho);
    }
}

void buzzer_beep_pattern()
{
    static bool buzzer_on = false;
    static uint64_t last_toggle_time = 0;
    uint64_t now = to_ms_since_boot(get_absolute_time());

    if (now - last_toggle_time >= 200)
    { // alterna a cada 200ms
        buzzer_on = !buzzer_on;
        gpio_put(BUZZER_PIN, buzzer_on);
        last_toggle_time = now;
    }
}
