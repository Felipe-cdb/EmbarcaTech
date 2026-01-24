#include <stdio.h>
#include "pico/stdlib.h"
// Biblioteca do sensor VL53L0X
#include "vl53l0x/vl53l0x.h"
// Biblioteca para BUZZER
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// --- Configuração das Porta I2C da BitDogLab ---
#define I2C_PORT_DIST i2c1
const uint DIST_SDA_PIN = 2;
const uint DIST_SCL_PIN = 3;

// --- Configuração do LED-RGB ---
#define LED_VERDE     11
#define LED_AZUL      12
#define LED_VERMELHO  13

//  --- Configuração do Buzzer ---
#define BUZZER_A_PIN 10
#define BUZZER_B_PIN 21
// Configuração da frequência do buzzer (em Hz)
#define BUZZER_FREQUENCY 2000

// -----------------------------------------------------------------------------
// ----------- Controle não bloqueante do LED -----------------------------------
// -----------------------------------------------------------------------------

typedef enum {
    LED_IDLE,
    LED_ERRO_ATIVO,
    LED_OK_ATIVO
} led_state_t;

static led_state_t led_state = LED_IDLE;
static absolute_time_t led_timeout;
static bool sensor_fora_alcance = false;

// -----------------------------------------------------------------------------
// ----------- Controle não bloqueante do BUZZER --------------------------------
// -----------------------------------------------------------------------------

typedef enum {
    BUZZER_IDLE,
    BUZZER_ATIVO
} buzzer_state_t;

typedef struct {
    uint pin;
    uint slice;
    buzzer_state_t state;
    absolute_time_t timeout;
} buzzer_t;

// -----------------------------------------------------------------------------
// ----------- Funções LED RGB ---------------------------------------------------
// -----------------------------------------------------------------------------
// Atualiza o estado do LED (deve ser chamada no loop principal)
void led_update(){
    if (led_state == LED_IDLE)
        return;

    if (absolute_time_diff_us(get_absolute_time(), led_timeout) <= 0){
        // Desliga os LEDs conforme o estado
        gpio_put(LED_VERDE, 0);
        gpio_put(LED_VERMELHO, 0);
        gpio_put(LED_AZUL, 0);

        led_state = LED_IDLE;
    }
}

// Função auxiliar para desligar todos os LEDs
static void led_clear_all(){
    gpio_put(LED_VERDE, 0);
    gpio_put(LED_AZUL, 0);
    gpio_put(LED_VERMELHO, 0);
}

// --- Função para indicar erro no sensor de distância ---
// (Não bloqueante)
void led_erro_sensorDistancia(){
    if (led_state == LED_ERRO_ATIVO)
        return;

    led_clear_all();

    gpio_put(LED_VERDE, 1);
    gpio_put(LED_VERMELHO, 1);

    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

// Acende o LED em ciano para indicar fora de alcance
// (Não bloqueante)
void led_sem_alcance_sensorDistancia(){
    if (led_state == LED_ERRO_ATIVO)
        return;

    led_clear_all();

    gpio_put(LED_VERDE, 1);
    gpio_put(LED_AZUL, 1);

    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}


// Acende o LED em Azul para indicar presença
// (Não bloqueante)
void led_disparo_sensorDistancia(){
    if (led_state == LED_OK_ATIVO)
        return;

    led_clear_all();

    gpio_put(LED_AZUL, 1);

    led_state = LED_OK_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

// -----------------------------------------------------------------------------
// ----------- Setup dos LEDs ---------------------------------------------------
// -----------------------------------------------------------------------------

void setup_leds(){
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);

    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
}

// -----------------------------------------------------------------------------
// ----------- Funções BUZZER --------------------------------------------------
// -----------------------------------------------------------------------------
void buzzer_init(buzzer_t* buzzer, uint pin) {
    buzzer->pin = pin;
    buzzer->slice = pwm_gpio_to_slice_num(pin);
    buzzer->state = BUZZER_IDLE;

    gpio_set_function(pin, GPIO_FUNC_PWM);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(
        &config,
        clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096.0f)
    );

    pwm_init(buzzer->slice, &config, true);
    pwm_set_gpio_level(pin, 0);
}


void buzzer_beep(buzzer_t* buzzer, uint duration_ms) {
    if (buzzer->state != BUZZER_IDLE)
        return;

    pwm_set_gpio_level(buzzer->pin, 2048); // 50% duty

    buzzer->state = BUZZER_ATIVO;
    buzzer->timeout = make_timeout_time_ms(duration_ms);
}

void buzzer_update(buzzer_t* buzzer) {
    if (buzzer->state == BUZZER_IDLE)
        return;

    if (absolute_time_diff_us(get_absolute_time(), buzzer->timeout) <= 0) {
        pwm_set_gpio_level(buzzer->pin, 0);
        buzzer->state = BUZZER_IDLE;
    }
}

// -----------------------------------------------------------------------------
// ----------- Setup do Sensor de Distância -------------------------------------
// -----------------------------------------------------------------------------

void setup_sensorDistancia(VL53L0X* sensorDistancia){
    if (!vl53l0x_init(sensorDistancia, I2C_PORT_DIST, DIST_SDA_PIN, DIST_SCL_PIN)){
        printf("ERRO: Falha ao inicializar o sensor VL53L0X.\n");
        // Emite sinal de erro com LED e para a execução
        while (1){
            led_erro_sensorDistancia();
            led_update();
            sleep_ms(10); // pequeno yield cooperativo
        }
    }

    // Inicia o modo de medição contínua (0ms = o mais rápido possível).
    vl53l0x_start_continuous(sensorDistancia, 0);
    sleep_ms(1000);
}

// -----------------------------------------------------------------------------
// ----------- Main -------------------------------------------------------------
// -----------------------------------------------------------------------------

int main(){
    // Inicializa stdio - todas interfaces de comunicação
    stdio_init_all();
    sleep_ms(2000); // Aguarda inicialização do console

    // Configuração dos LEDs
    setup_leds();

    // Garante todos os LEDs desligados inicialmente
    gpio_put(LED_VERDE, 0);
    gpio_put(LED_AZUL, 0);
    gpio_put(LED_VERMELHO, 0);

    // Inicializa os buzzers
    buzzer_t buzzerA;
    buzzer_t buzzerB;
    buzzer_init(&buzzerA, BUZZER_A_PIN);
    buzzer_init(&buzzerB, BUZZER_B_PIN);

    // Inicializa o sensor de Distância
    VL53L0X sensorDistancia;
    setup_sensorDistancia(&sensorDistancia);
    uint16_t ult_distance = 0;

    // Loop principal
    while (true){
        uint16_t distance_cm = vl53l0x_read_continuous_cm(&sensorDistancia);

        // Timeout / erro bruto
        if (distance_cm == 6553){
            printf("Timeout ou erro de leitura.\n");
            led_erro_sensorDistancia();
            led_update();
            sleep_ms(10);
            continue;
        }

        // ----------------- FORA DE ALCANCE -----------------
        if (distance_cm > 800){
            if (!sensor_fora_alcance){
                printf("Fora de alcance.\n");
                sensor_fora_alcance = true;
            }

            led_sem_alcance_sensorDistancia();
            led_update();
            sleep_ms(10);
            continue;
        }

        // ----------------- VOLTOU PARA ZONA VÁLIDA -----------------
        if (sensor_fora_alcance){
            printf("Sensor voltou para zona detectável.\n");
            sensor_fora_alcance = false;
        }

        // ----------------- ZONA VÁLIDA -----------------
        printf("Distancia: %d cm\n", distance_cm);

        if (distance_cm <= 20){
            printf("Presença detectada.\n");
            led_disparo_sensorDistancia();
            buzzer_beep(&buzzerA, 200);
            buzzer_beep(&buzzerB, 200);
        }

        buzzer_update(&buzzerA);
        buzzer_update(&buzzerB);
        led_update();
        sleep_ms(10);
    }
}
