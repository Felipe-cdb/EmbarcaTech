#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define SERVO_PIN 0   // GP0 (SDA no conector I2C0)
// #define SERVO_PIN 2   // GP2 (SDA no conector I2C1)

// Configura o PWM para controlar o servo(pulso do servo em microssegundos)
void servo_set_us(uint slice, uint channel, uint32_t us) {
    uint32_t clock = 125000000;  // 125 MHz clock
    uint32_t wrap = clock / 50 / 64 - 1; // 50 Hz (20ms)
    pwm_set_wrap(slice, wrap);

    // Duty proporcional ao tempo do pulso (us)
    uint32_t level = (us * (wrap + 1)) / 20000;
    // 1500 µs → servo parado
    // 1000 µs → giro máximo sentido horario
    // 2000 µs → giro máximo no sentido anti-horario
    pwm_set_chan_level(slice, channel, level);
}

int main() {
    stdio_init_all();
    // Configura o pino escolhido para função PWM, porque por padrão todo pino é GPIO simples.
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);

    // divide o clock de 125 MHz por 64, facilitando calcular o wrap para 20 ms.
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    // habilita o PWM para começar a gerar os pulsos.
    uint channel = pwm_gpio_to_channel(SERVO_PIN);
    

    pwm_set_clkdiv(slice, 64.0f); 
    pwm_set_enabled(slice, true);

    while (1) {
        // Parado
        servo_set_us(slice, channel, 1500);
        sleep_ms(2000);

        // Gira em um sentido horario
        servo_set_us(slice, channel, 1100);
        sleep_ms(2000);

        // Parado
        servo_set_us(slice, channel, 1500);
        sleep_ms(2000);

        // Gira no outro sentido anti-horario
        servo_set_us(slice, channel, 1900);
        sleep_ms(2000);
    }
}
