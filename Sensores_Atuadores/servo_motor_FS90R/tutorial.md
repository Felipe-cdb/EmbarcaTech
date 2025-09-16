# Servo contínuo (ex.: FS90R)
Não tem posição fixa, só gira continuamente.
O pulso PWM determina a velocidade e a direção, não a posição.
* 1500 µs → parado
servo_stop(pin_servo);
* 1000 µs → horário rápido
servo_clockwise(pin_servo,speed);  // velocidade de 0 a 100%
* 2000 µs → anti-horário rápido
servo_counterClockwise(pin_servo,speed);  // velocidade de 0 a 100%

# ⚡ Boas práticas
O FS90R é um servo de rotação contínua, e se girar sempre na velocidade máxima (PWM 1000 µs ou 2000 µs) pode ter alguns efeitos negativos a longo prazo

* Evitar manter o servo sempre no extremo (1000 ou 2000 µs).

* Se possível, usar valores intermediários (1100–1900 µs) para reduzir desgaste e calor.

* Garantir boa alimentação (3.3 V está ok, mas se perceber fraco, um conversor 5 V estável é melhor).

* Permitir pausas ou mudanças de direção para não ficar rodando sempre no mesmo sentido.

# Simular “graus” em servo contínuo
Se você quer “simular graus” com FS90R, precisa controlar o tempo de acionamento:

* Calcule a o tempo e velocidade para uma volta.
servo_calibrate_manual(Servo *servo, uint8_t speed, uint32_t test_time_ms);
ex: servo_calibrate_manual(&meu_servo, 100, 1850);
    sleep_ms(3000);

* Determine o tempo de rotação necessário para mover uma fração da volta
servo_turn_degrees(pin_servo, speed, degrees, clockwise);
servo_turn_degrees(pin_servo, 70, 90, true); // gira 90° com velocidade de 70% sentido horário

servo_turn_degrees(pin_servo, 80, 90, false); // gira 90° com velocidade de 80% sentido anti-horário
