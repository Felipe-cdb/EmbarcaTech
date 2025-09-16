#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "fs90r/fs90r.h"    // funções da biblioteca FS90R

#define SERVO_PIN 0

int main() {
    stdio_init_all();

    Servo meu_servo = servo_init(SERVO_PIN);

    // Calibração manual: ajuste conforme o seu servo
    servo_calibrate_manual(&meu_servo, 100, 1750);
    sleep_ms(3000);

    while(1) {
        servo_turn_degrees(&meu_servo, 100, 90, true);   // 90° horário
        sleep_ms(1000);

        servo_turn_degrees(&meu_servo, 100, 90, false);  // 90° anti-horário
        sleep_ms(1000);
    }
}
