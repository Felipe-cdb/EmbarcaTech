#include "fs90r.h"

// Faixas seguras de pulso em microssegundos
#define US_MIN_CLOCKWISE 1100
#define US_MAX_CLOCKWISE 1500
#define US_MIN_COUNTERCLOCKWISE 1500
#define US_MAX_COUNTERCLOCKWISE 1900

// ----------------- Função auxiliar -----------------
static void servo_set_us(Servo *servo, uint32_t us) {
    if(us < 1000) us = 1000; // nunca abaixo de 1000 µs
    if(us > 2000) us = 2000; // nunca acima de 2000 µs
    uint32_t level = (us * (servo->wrap + 1)) / 20000;
    pwm_set_chan_level(servo->slice, servo->channel, level);
}

// ----------------- Inicialização -----------------
Servo servo_init(uint pin) {
    Servo servo;
    servo.pin = pin;
    gpio_set_function(pin, GPIO_FUNC_PWM);
    servo.slice = pwm_gpio_to_slice_num(pin);
    servo.channel = pwm_gpio_to_channel(pin);
    pwm_set_clkdiv(servo.slice, 64.0f);
    servo.wrap = 125000000 / 50 / 64 - 1; // 50 Hz
    pwm_set_wrap(servo.slice, servo.wrap);
    pwm_set_enabled(servo.slice, true);

    servo.rotation_time_per_360 = 0; // ainda não calibrado
    servo_stop(&servo);
    return servo;
}

// ----------------- Controle básico -----------------
void servo_stop(Servo *servo) {
    servo_set_us(servo, 1500);
}

void servo_clockwise(Servo *servo, uint8_t speed) {
    if(speed > 100) speed = 100;
    uint32_t us = US_MAX_CLOCKWISE - ((US_MAX_CLOCKWISE - US_MIN_CLOCKWISE) * speed) / 100;
    servo_set_us(servo, us);
}

void servo_counterclockwise(Servo *servo, uint8_t speed) {
    if(speed > 100) speed = 100;
    uint32_t us = US_MIN_COUNTERCLOCKWISE + ((US_MAX_COUNTERCLOCKWISE - US_MIN_COUNTERCLOCKWISE) * speed) / 100;
    servo_set_us(servo, us);
}

// ----------------- Calibração manual -----------------
void servo_calibrate_manual(Servo *servo, uint8_t speed, uint32_t test_time_ms) {
    servo_clockwise(servo, speed);
    sleep_ms(test_time_ms);
    servo_stop(servo);
    servo->rotation_time_per_360 = (float)test_time_ms;
}

// ----------------- Giro por graus -----------------
void servo_turn_degrees(Servo *servo, uint8_t speed, float degrees, bool clockwise) {
    if(servo->rotation_time_per_360 <= 0) return; // ainda não calibrado

    if(speed > 100) speed = 100;
    if(speed < 0) speed = 0;

    uint32_t time_to_turn = (uint32_t)((degrees / 360.0f) * servo->rotation_time_per_360);

    if(clockwise)
        servo_clockwise(servo, speed);
    else
        servo_counterclockwise(servo, speed);

    sleep_ms(time_to_turn);
    servo_stop(servo);
}
