biblioteca criada por Felipe Correia 23/09/2025

# Servo contínuo (ex.: FS90R)
Não tem posição fixa, só gira continuamente.
O pulso PWM determina a velocidade e a direção, não a posição.

# Funções disponiveis na biblioteca
Controle básico

servo_init() → inicializa com pulso neutro (stop).
servo_clockwise() → gira horário ajustando velocidade.
servo_counterClockwise() → gira anti-horário ajustando velocidade.
servo_stop() → envia pulso neutro para parar.

Calibração
servo_calibrate_manual() → usuário mede manualmente quanto tempo demora 360°.

Movimento por graus
servo_turn_degrees() → gira X graus baseado no tempo calibrado.

# Detalhes do controle básico
* 1500 µs → parado
// utilize essa função para manter servo parado
servo_stop(pin_servo); 
ex:servo_stop(&meu_servo); // parado

* 1000 µs → horário rápido
// Função para girar servo no sentido horario a velocidade de 0 a 100%
servo_clockwise(pin_servo,speed);  
ex: servo_clockwise(&meu_servo, 50); //horario a 50%

* 2000 µs → anti-horário rápido
// Função para girar servo no sentido anti-horario a velocidade de 0 a 100%
servo_counterClockwise(pin_servo,speed);
ex: servo_counterclockwise(&meu_servo, 100); //anti-horario a 100%

# Simular “graus” em servo contínuo
Se você quer “simular graus” com FS90R, precisa controlar o tempo de acionamento:

* Calcule o tempo e velocidade para uma volta completa.
servo_calibrate_manual(Servo *servo, uint8_t speed, uint32_t test_time_ms);
ex: servo_calibrate_manual(&meu_servo, 100, 1850);
    sleep_ms(3000);

* Com servo calibrado obtemos o rotation_time_per_360 que é utilizado nas funções que simula grau. OBS: a função de simulçao precisa tá com a mesma porcentagem de velocidade da calibragem.
parametros: pino do servo, velocidade (0 a 100%), graus, e true(para sentido horario) ou falso(para sentido antihorario)
servo_turn_degrees(pin_servo, speed, degrees, true/false);

ex:
servo_turn_degrees(pin_servo, 70, 90, true); // gira 90° com velocidade de 70% sentido horário

servo_turn_degrees(pin_servo, 80, 90, false); // gira 90° com velocidade de 80% sentido anti-horário


# CMakeList
Precisei do seguites configurações em meu cmakerlist: 
    # Add the standard library to the build
    target_link_libraries(servo_motor_FS90R
            pico_stdlib)

    # Add the standard include files to the build
    target_include_directories(servo_motor_FS90R PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${CMAKE_CURRENT_LIST_DIR}/lib
    )

    # Adicionar os arquivos da biblioteca ao projeto
    target_sources(servo_motor_FS90R PRIVATE
            lib/fs90r/fs90r.c
    )

    # Add any user requested libraries
    target_link_libraries(servo_motor_FS90R 
            hardware_pwm        
    )

# ⚡ Boas práticas
O FS90R é um servo de rotação contínua, e se girar sempre na velocidade máxima (PWM 500us–2500us) pode ter alguns efeitos negativos a longo prazo

* Evitar manter o servo sempre no extremo (500 – 2500us).

* Se possível, usar valores intermediários (1000–2000 µs) para reduzir desgaste e calor.

* Garantir boa alimentação (3.3 V está ok, mas se perceber fraco, um conversor 5 V estável é melhor).

* Permitir pausas ou mudanças de direção para não ficar rodando sempre no mesmo sentido.

