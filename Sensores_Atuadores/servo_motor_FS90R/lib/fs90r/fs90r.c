#include "fs90r.h"

// ----------------- Inicialização -----------------
Servo servo_init(uint pin) {
    Servo servo;
    servo.pin = pin;
    
    // Configuração PWM simplificada
    gpio_set_function(servo.pin, GPIO_FUNC_PWM);  // Definindo o pino para PWM
    servo.slice = pwm_gpio_to_slice_num(servo.pin); // Obtendo o slice correspondente ao pino
    servo.channel = pwm_gpio_to_channel(servo.pin); // Obtendo o canal PWM para o pino

    pwm_config cfg = pwm_get_default_config();     // Obtendo a configuração padrão
    pwm_config_set_clkdiv(&cfg, 125.0f);           // Divisor de clock para 1µs por tick
    pwm_config_set_wrap(&cfg, 20000 - 1);          // Definindo wrap para 50 Hz (20 ms)
    
    pwm_init(servo.slice, &cfg, true);                    // Inicia o PWM com a configuração
    
    // Inicialização do tempo de rotação (pode ser calibrado depois)
    servo.rotation_time_per_360 = 0;
    
    // Inicializando o servo na posição neutra
    servo_stop(&servo);
    
    return servo;
}

// ----------------- Função auxiliar -----------------
static void servo_pulse_us(Servo *servo, uint32_t us) {
    if(us < 700) us = 1000; // nunca abaixo de 1000 µs
    if(us > 2000) us = 2000; // nunca acima de 2000 µs
    pwm_set_chan_level(servo->slice, servo->channel, us);
}

// ----------------- Controle básico -----------------
void servo_stop(Servo *servo) {
    servo_pulse_us(servo, US_NEUTRAL);
}

void servo_clockwise(Servo *servo, uint8_t percentage_speed) {
    if(percentage_speed > 100) percentage_speed = 100;
    uint32_t us = US_NEUTRAL - ((US_NEUTRAL - US_MIN_PULSE) * percentage_speed) / 100;
    servo_pulse_us(servo, us);
}

void servo_counterclockwise(Servo *servo, uint8_t percentage_speed) {
    if(percentage_speed > 100) percentage_speed = 100;
    uint32_t us = US_NEUTRAL + ((US_MAX_PULSE - US_NEUTRAL) * percentage_speed) / 100;
    servo_pulse_us(servo, us);
}

// ----------------- Calibração manual -----------------
void servo_calibrate_manual(Servo *servo, uint8_t percentage_speed, uint32_t test_time_ms) {
    servo_clockwise(servo, percentage_speed);
    sleep_ms(test_time_ms);
    servo_stop(servo);
    servo->rotation_time_per_360 = (float)test_time_ms;
}

// ----------------- Giro por graus -----------------
void servo_turn_degrees(Servo *servo, uint8_t percentage_speed, float degrees, bool clockwise) {
    if(servo->rotation_time_per_360 <= 0) return; // ainda não calibrado

    if(percentage_speed > 100) percentage_speed = 100;
    if(percentage_speed < 0) percentage_speed = 0;

    uint32_t time_to_turn = (uint32_t)((degrees / 360.0f) * servo->rotation_time_per_360);

    if(clockwise)
        servo_clockwise(servo, percentage_speed);
    else
        servo_counterclockwise(servo, percentage_speed);

    sleep_ms(time_to_turn);
    servo_stop(servo);
}
