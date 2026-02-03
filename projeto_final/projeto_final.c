#include <stdio.h>
// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
// Biblioteca padrão do Pico
#include "pico/stdlib.h"
// Biblioteca do sensor VL53L0X
#include "vl53l0x/vl53l0x.h"
// Biblioteca para BUZZER
#include "hardware/pwm.h"
#include "hardware/clocks.h"
// Biblioteca para Display OLED
#include <string.h>             // memset
#include <ctype.h>              // toupper
#include "hardware/i2c.h"       // i2c_init e controle do I2C
#include "ssd1306/ssd1306.h"    // funções da biblioteca SSD1306
// Biblioteca para AHT10 - temperatura e umidade
#include "aht10/aht10.h"

// --- Configuração das Porta I2C para Sensor Distancia e AHT10---
#define I2C_DIST_AHT10 i2c0
const uint DIST_AHT10_SDA_PIN = 0;
const uint DIST_AHT10_SCL_PIN = 1;

#define DIST_MAX_VALIDA 819

// --- Definição dos pinos I2C1 usados para o display ---
#define I2C_PORT_DISPLAY i2c1
const uint DISPLAY_SDA_PIN = 14;
const uint DISPLAY_SCL_PIN = 15;

// --- Configuração do LED-RGB ---
#define LED_VERDE     11
#define LED_AZUL      12
#define LED_VERMELHO  13

//  --- Configuração do Buzzer ---
#define BUZZER_A_PIN 10
#define BUZZER_B_PIN 21
// Configuração da frequência do buzzer (em Hz)
#define BUZZER_FREQUENCY 2000

// --- Botão de compra ---
#define BOTAO_COMPRA_PIN 5

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

buzzer_t buzzerA;
buzzer_t buzzerB;
// -----------------------------------------------------------------------------
// ----------- Controle do Display OLED SSD1306 --------------------------------
// -----------------------------------------------------------------------------

static uint8_t ssd[ssd1306_buffer_length];

static struct render_area frame_area = {
    .start_column = 0,
    .end_column   = ssd1306_width - 1,
    .start_page   = 0,
    .end_page     = ssd1306_n_pages - 1
};

// -----------------------------------------------------------------------------
// ----------- Event Group do Sistema ------------------------------------------
// -----------------------------------------------------------------------------

EventGroupHandle_t system_events;

// -----------------------------------------------------------------------------
// ----------- EventGroup - Bits do Sistema ------------------------------------
// -----------------------------------------------------------------------------

#define EVT_ERRO_SENSOR_DIST   (1 << 0)
#define EVT_ERRO_SENSOR_AHT10  (1 << 1)
#define EVT_ERRO_GERAL         (1 << 2)

#define EVT_SEM_ALCANCE        (1 << 3)
#define EVT_VISITANTE_DETECT  (1 << 4)
#define EVT_COMPRA_REALIZADA  (1 << 5)

#define EVT_UI_CLEAR          (1 << 6)



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

// --- Led para erro geral ---
void led_erro_geral(){
    if (led_state == LED_ERRO_ATIVO)
        return;

    led_clear_all();

    gpio_put(LED_VERMELHO, 1);

    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

// ----- LEDS para sensor de distância ----
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

// ----- LEDS para sensor de Temperatura e Umidade AHT10 ----
void led_erro_sensorAHT10(){
    if (led_state == LED_ERRO_ATIVO)
        return;

    led_clear_all();

    gpio_put(LED_AZUL, 1);
    gpio_put(LED_VERMELHO, 1);

    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

// --- Led compra realizada com sucesso ---
void led_compra(){
    if (led_state == LED_OK_ATIVO)
        return;

    led_clear_all();

    gpio_put(LED_VERDE, 1);

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
// ----------- Setup do Display OLED --------------------------------------------
// -----------------------------------------------------------------------------

void setup_displayOLED(){
    // Inicializa o I2C para o display
    i2c_init(I2C_PORT_DISPLAY, ssd1306_i2c_clock * 1000);
    gpio_set_function(DISPLAY_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA_PIN);
    gpio_pull_up(DISPLAY_SCL_PIN);

    // Inicializa o display
    ssd1306_init();

    calculate_render_area_buffer_length(&frame_area);

    // Limpa tela
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Tela inicial
    char *text[] = {
        " Bem-vindos!   ",
        " Sistema de    ",
        " Monitoramento ",
        " Iniciando...  ",
        "        ______ ",
        "       |(.)(.)|",
        "       |  --  |",
        "       |      |",
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

// -----------------------------------------------------------------------------
// ----------- Funções de Mensagens no Display ---------------------------------
// -----------------------------------------------------------------------------

void display_info_gerais(uint16_t distance_cm, float temperature, float humidity){
    memset(ssd, 0, ssd1306_buffer_length);

    char linha1[32];
    sprintf(linha1, "Distancia = %d", distance_cm);
    char linha2[32];
    sprintf(linha2, "Temperatura: %.2f C", temperature);
    char linha3[32];
    sprintf(linha3, "Umidade: %.2f %%", humidity);

    char *text[] = {
        linha1,
        "               ",
        "     ...       ",
        "  Aguardando   ",
        "   Visitante   ",
        "     ...       ",
        linha2,
        linha3,
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

void display_erro_sensorDistancia(){
    memset(ssd, 0, ssd1306_buffer_length);

    char *text[] = {
        "   ERRO!       ",
        "               ",
        " Verifique se  ",
        " o Sensor de   ",
        " Distancia     ",
        " esta          ",
        " danificado!   ",
        "               ",
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

void display_sem_alcance_sensorDistancia(){
    memset(ssd, 0, ssd1306_buffer_length);


    char *text[] = {
        "               ",
        "    Ajuste     ",
        "  Corretamente ",
        "  a Distancia  ",
        "  de Entrada!  ",
        "               ",
        " Ate Led Ciano ",
        "     Apagar!   ",
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

void display_disparo_sensorDistancia(){
    memset(ssd, 0, ssd1306_buffer_length);

    char *text[] = {
        "               ",
        "               ",
        "               ",
        "  Visitante    ",
        "  Detectado!   ",
        "               ",
        "               ",
        "               ",
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

void display_erro_sensorAHT10(){
    memset(ssd, 0, ssd1306_buffer_length);

    char *text[] = {
        "   ERRO!       ",
        "               ",
        " Verifique se  ",
        " o Sensor de   ",
        " Temperatura   ",
        " esta          ",
        " danificado!   ",
        "               ",
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

void compra_display(){
    memset(ssd, 0, ssd1306_buffer_length);

    char *text[] = {
        "               ",
        "   COMPRA      ",
        "   Registrada   ",
        "   COM SUCESSO ",
        "               ",
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, &frame_area);
}

// -----------------------------------------------------------------------------
// ----------- Setup do I2C0 para AHT10 e VL53L0X ------------------------------
// ----------------------------------------------------------------------------- 
void setup_i2c0() { 
    i2c_init(I2C_DIST_AHT10, 400 * 1000);
    gpio_set_function(DIST_AHT10_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DIST_AHT10_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DIST_AHT10_SDA_PIN);
    gpio_pull_up(DIST_AHT10_SCL_PIN); 
}

// -----------------------------------------------------------------------------
// ----------- Setup do Sensor de Distância ------------------------------------
// -----------------------------------------------------------------------------
VL53L0X sensorDistancia;
bool setup_sensorDistancia(VL53L0X* sensorDistancia){
    if (!vl53l0x_init(sensorDistancia, I2C_DIST_AHT10, DIST_AHT10_SDA_PIN, DIST_AHT10_SCL_PIN)){
        xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_DIST);
        return false;
    }
    vl53l0x_start_continuous(sensorDistancia, 0);
    return true;
}

// -----------------------------------------------------------------------------
// ----------- Setup do Sensor de Temperatura e Umidade AHT10 ------------------
// -----------------------------------------------------------------------------
AHT10 sensorAHT10;
bool setup_sensorAHT10(AHT10* sensorAHT10){
    if (!aht10_init(sensorAHT10, I2C_DIST_AHT10, DIST_AHT10_SDA_PIN, DIST_AHT10_SCL_PIN)) {
        xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_AHT10);
        return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
// ----------- Setup do Botão de compra ----------------------------------------
// -----------------------------------------------------------------------------
void setup_botao_compra(){
    gpio_init(BOTAO_COMPRA_PIN);
    gpio_set_dir(BOTAO_COMPRA_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_COMPRA_PIN); // Habilita o resistor pull-up interno
}

// -----------------------------------------------------------------------------
// ----------- Funções do Botão de compra --------------------------------------
// -----------------------------------------------------------------------------
void compra_registrada() {
    static bool last_state = true;
    bool current = gpio_get(BOTAO_COMPRA_PIN);

    if (last_state && !current) {
        xEventGroupSetBits(system_events, EVT_COMPRA_REALIZADA);
    }

    last_state = current;
}

// -----------------------------------------------------------------------------
// ----------- Tarefa de gestão de Inteface LED e Display ----------------------
// -----------------------------------------------------------------------------

void task_ui(void *pvParameters) {
    (void) pvParameters;

    EventBits_t events;

    for (;;) {

        events = xEventGroupWaitBits(
            system_events,
            EVT_ERRO_SENSOR_DIST |
            EVT_ERRO_SENSOR_AHT10 |
            EVT_ERRO_GERAL |
            EVT_SEM_ALCANCE |
            EVT_VISITANTE_DETECT |
            EVT_COMPRA_REALIZADA |
            EVT_UI_CLEAR,
            pdTRUE,     // limpa bits ao sair
            pdFALSE,    // qualquer bit
            portMAX_DELAY
        );

        // ---------------- ERROS ----------------
        if (events & EVT_ERRO_SENSOR_DIST) {
            led_erro_sensorDistancia();
            display_erro_sensorDistancia();
        }
        else if (events & EVT_ERRO_SENSOR_AHT10) {
            led_erro_sensorAHT10();
            display_erro_sensorAHT10();
        }
        else if (events & EVT_ERRO_GERAL) {
            led_erro_geral();
        }

        // ---------------- ESTADOS ----------------
        else if (events & EVT_SEM_ALCANCE) {
            led_sem_alcance_sensorDistancia();
            display_sem_alcance_sensorDistancia();
        }
        else if (events & EVT_VISITANTE_DETECT) {
            led_disparo_sensorDistancia();
            display_disparo_sensorDistancia();
        }
        else if (events & EVT_COMPRA_REALIZADA) {
            led_compra();
            compra_display();
        }

        // loop de manutenção
        for (int i = 0; i < 5; i++) {
            led_update();
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


// -----------------------------------------------------------------------------
// ----------- Tarefa Principal do FreeRTOS ------------------------------------
// -----------------------------------------------------------------------------
void task_main(void *pvParameters) {
    (void) pvParameters;

    for (;;) {

        // ---------------------------------------------------------------------
        // Leitura dos sensores
        // ---------------------------------------------------------------------
        uint16_t distance_cm = vl53l0x_read_continuous_cm(&sensorDistancia);

        float temperature = aht10_get_temperature(&sensorAHT10);
        float humidity    = aht10_get_humidity(&sensorAHT10);
        
        // ---------------------------------------------------------------------
        // Tratamento de erro - Sensor de Distância
        // ---------------------------------------------------------------------
        static bool sem_alcance_ativo = false;
        if (distance_cm > DIST_MAX_VALIDA) {
            if (!sem_alcance_ativo) {
                xEventGroupSetBits(system_events, EVT_SEM_ALCANCE);
                sem_alcance_ativo = true;
            }
        } else {
            sem_alcance_ativo = false;
        }

        if (distance_cm == 6553 || distance_cm == 0) {
            xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_DIST);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // ---------------------------------------------------------------------
        // Sensor fora de alcance
        // ---------------------------------------------------------------------
        // Sensor fora do range configurado (estado, não erro)
        if (distance_cm > DIST_MAX_VALIDA) {
            xEventGroupSetBits(system_events, EVT_SEM_ALCANCE);
        } else if (distance_cm <= 50) {
            xEventGroupSetBits(system_events, EVT_VISITANTE_DETECT);
            buzzer_beep(&buzzerA, 200);
            buzzer_beep(&buzzerB, 200);
        }


        // ---------------------------------------------------------------------
        // Tratamento de erro - Sensor AHT10
        // ---------------------------------------------------------------------
        if (temperature <= -1000.0f || humidity < 0.0f) {
            xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_AHT10);
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // ---------------------------------------------------------------------
        // Compra registrada (botão)
        // ---------------------------------------------------------------------
        compra_registrada();
        
        // ---------------------------------------------------------------------
        // Atualização de atuadores não bloqueantes
        // ---------------------------------------------------------------------
        buzzer_update(&buzzerA);
        buzzer_update(&buzzerB);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// -----------------------------------------------------------------------------
// ----------- Main -------------------------------------------------------------
// -----------------------------------------------------------------------------

int main(){
    // Inicializa stdio - todas interfaces de comunicação
    stdio_init_all();

    // Cria o Event Group do sistema
    system_events = xEventGroupCreate();

    if (system_events == NULL) {
        // Falha crítica de sistema
        xEventGroupSetBits(system_events, EVT_ERRO_GERAL);
    }

    // Configuração dos LEDs
    setup_leds();

    // Garante todos os LEDs desligados inicialmente
    led_clear_all();

    // Configuração do botão de compra
    setup_botao_compra();

    // Inicializa os buzzers
    buzzer_init(&buzzerA, BUZZER_A_PIN);
    buzzer_init(&buzzerB, BUZZER_B_PIN);

    // Inicializa o display OLED
    setup_displayOLED();

    // Configuração do I2C0 para sensores de distância e AHT10
    setup_i2c0();

    // Inicializa o sensor de Distância
    setup_sensorDistancia(&sensorDistancia);

    // Inicializa o sensor de Temperatura e Umidade AHT10
    setup_sensorAHT10(&sensorAHT10);

    // Loop principal
    xTaskCreate(
        task_main,
        "MainTask",
        2048,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    xTaskCreate(
        task_ui,
        "UITask",
        2048,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );


    vTaskStartScheduler();

    // Nunca deve chegar aqui
    while (true) {}
}
