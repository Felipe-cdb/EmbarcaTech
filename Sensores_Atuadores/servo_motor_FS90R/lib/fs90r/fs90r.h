#ifndef FS90R_H
#define FS90R_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

// Limites do FS90R: 500–2500us
// As faixas de pulso seguras para o servo são de 1000–2000us.
// US_MIN_PULSE (1000us) define o limite para o movimento horário máximo.
// US_NEUTRAL (1500us) define a posição neutra (parada).
// US_MAX_PULSE (2000us) define o limite para o movimento anti-horário máximo.
#define US_MIN_PULSE 1000 
#define US_NEUTRAL 1500 
#define US_MAX_PULSE 2000 


// Estrutura do servo
typedef struct {
    uint pin;
    uint slice;
    uint channel;
    uint32_t wrap;
    float rotation_time_per_360; // tempo de 1 volta em ms (por servo)
} Servo;

// Inicializa o servo em um pino
Servo servo_init(uint pin);

// Controle básico do servo
void servo_stop(Servo *servo); // Para o servo
void servo_clockwise(Servo *servo, uint8_t percentage_speed); // Gira no sentido horário com velocidade percentual (0-100%)
void servo_counterclockwise(Servo *servo, uint8_t percentage_speed); // Gira no sentido anti-horário com velocidade percentual (0-100%)

// Calibração manual: roda continuamente e espera tempo estimado para 1 volta completa na velocidade escolhida, salva o tempo em servo->rotation_time_per_360
void servo_calibrate_manual(Servo *servo, uint8_t percentage_speed, uint32_t test_time_ms);

// Simula graus girando na direção escolhida com a velocidade escolhida (0-100%), precisa estar calibrado, essa função apenas simula os graus, não é uma função precisa
void servo_turn_degrees(Servo *servo, uint8_t percentage_speed, float degrees, bool clockwise);

#endif
