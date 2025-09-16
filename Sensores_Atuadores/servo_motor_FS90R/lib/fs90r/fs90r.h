#ifndef FS90R_H
#define FS90R_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

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
void servo_stop(Servo *servo);                     // Para o servo
void servo_clockwise(Servo *servo, uint8_t speed); // 0–100%
void servo_counterclockwise(Servo *servo, uint8_t speed); // 0–100%

// Calibração manual: roda continuamente e espera tempo estimado para 1 volta
void servo_calibrate_manual(Servo *servo, uint8_t speed, uint32_t test_time_ms);

// Gira X graus usando tempo calibrado
void servo_turn_degrees(Servo *servo, uint8_t speed, float degrees, bool clockwise);

#endif
