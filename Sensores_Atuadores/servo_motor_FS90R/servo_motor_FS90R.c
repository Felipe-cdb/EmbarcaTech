#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "fs90r/fs90r.h"    // funções da biblioteca FS90R

#define SERVO_PIN 0

int main() {
    stdio_init_all();
    sleep_ms(1000); // Aguarda a inicialização do terminal serial

    //Instaciando o servo
    Servo meu_servo = servo_init(SERVO_PIN);

    printf("Testando funções de rotação \n");
    servo_clockwise(&meu_servo, 50); // gira no sentido horário a 50% de velocidade
    sleep_ms(2000);
    servo_stop(&meu_servo); // para o servo
    sleep_ms(1000);

    servo_counterclockwise(&meu_servo, 100); // gira no sentido anti-horário a 50% de velocidade
    sleep_ms(2000);
    servo_stop(&meu_servo); // para o servo
    sleep_ms(1000);

    // servo_calibrate_manual(&meu_servo, porcentagem de velocidade, tempo para 1 volta); 
    // Calibração manual: roda a 50% de velocidade por 2235 ms (tempo estimado para 1 volta completa)
    // testar tempo até o servo completar 1 volta completa
    printf("Calibrando servo...\n");
    servo_calibrate_manual(&meu_servo, 50, 2235);
    sleep_ms(2000);
    printf("Servo calibrado. Tempo para 1 volta: %.2f ms\n", meu_servo.rotation_time_per_360);
    servo_stop(&meu_servo);
    sleep_ms(1000);
    

    while(1) {

        printf("Girando 90 graus no sentido horário...\n");
        servo_turn_degrees(&meu_servo, 50, 90, true); // gira 90 graus no sentido horário a 50% de velocidade
        sleep_ms(1000);
        printf("Girando 90 graus no sentido anti-horário...\n");
        servo_turn_degrees(&meu_servo, 50, 90, false); // gira 90 graus no sentido anti-horário a 50% de velocidade
        sleep_ms(1000);        
    }
    return 0;
}
