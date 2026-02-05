#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Wi-Fi Pico W
#include "pico/cyw43_arch.h"
#include "lwipopts.h"

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

// Raspberry Pi Pico
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"

// Sensores e Display
#include "vl53l0x/vl53l0x.h"
#include "aht10/aht10.h"
#include "ssd1306/ssd1306.h"

// -----------------------------------------------------------------------------
// ----------- Definições e Configurações de Pinos ------------------------------
// -----------------------------------------------------------------------------

// I2C0: Sensores VL53L0X e AHT10
#define I2C_DIST_AHT10 i2c0
const uint DIST_AHT10_SDA_PIN = 0;
const uint DIST_AHT10_SCL_PIN = 1;

// I2C1: Display OLED
#define I2C_PORT_DISPLAY i2c1
const uint DISPLAY_SDA_PIN = 14;
const uint DISPLAY_SCL_PIN = 15;

// LED RGB
#define LED_VERDE     11
#define LED_AZUL      12
#define LED_VERMELHO  13

// Buzzer
#define BUZZER_A_PIN 10
#define BUZZER_B_PIN 21
#define BUZZER_FREQUENCY 2000

// Botão de Compra
#define BOTAO_COMPRA_PIN 5

// distância máxima que o sensor vl53l0x considera válida (em cm)
#define DIST_MAX_VALIDA 800
// distância para detectar visitante (em cm) - considere tamanho do portal e coloque um valor uns 20 cm menor
#define DIST_VISITANTE_CM 50 
// intervalo de envio de temperatura via HTTP (1 minutos)
#define TEMPO_ENVIO_TEMPERATURA_MS (1 * 60 * 1000)



// -----------------------------------------------------------------------------
// ----------- Configuração Wi-Fi -----------------------------------------------
// -----------------------------------------------------------------------------

#define WIFI_SSID       "brisa-4338675"
#define WIFI_PASSWORD   "abqkbrvg"

// -----------------------------------------------------------------------------
// ----------- FreeRTOS Static Memory - Uso um FreeRTOS especifico -------------
// -----------------------------------------------------------------------------

// ====== FreeRTOS Static Memory ======
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB; *ppxIdleTaskStackBuffer = uxIdleTaskStack; *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB; *ppxTimerTaskStackBuffer = uxTimerTaskStack; *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

// -----------------------------------------------------------------------------
// ----------- Estruturas de Dados e Handlers ----------------------------------
// -----------------------------------------------------------------------------

// Event Group Bits
#define EVT_ERRO_SENSOR_DIST   (1 << 0)
#define EVT_ERRO_SENSOR_AHT10  (1 << 1)
#define EVT_ERRO_GERAL         (1 << 2)
#define EVT_SEM_ALCANCE        (1 << 3)
#define EVT_VISITANTE_DETECT   (1 << 4)
#define EVT_COMPRA_REALIZADA   (1 << 5)
#define EVT_BUZZER_BEEP        (1 << 7)
#define EVT_UI_UPDATE          (1 << 8)
#define EVT_WIFI_CONNECTED     (1 << 9)
#define EVT_WIFI_ERROR         (1 << 10)

// estrutura do estado do sistema
typedef struct {
    uint16_t distance_cm;
    float temperature;
    float humidity;
    bool dados_validos;
} system_state_t;

// Mensagens das filas VL53L0X e AHT10
typedef struct {
    uint16_t distance_cm;
} vl53_msg_t;

typedef struct {
    float temperature;
    float humidity;
} aht10_msg_t;

// Estruturas de mensagens HTTP
typedef enum {
    HTTP_EVT_VISITA,
    HTTP_EVT_COMPRA,
    HTTP_EVT_TEMPERATURA
} http_event_type_t;

typedef struct {
    http_event_type_t type;
    uint32_t value_u32;   // visitas ou compras
    float temperature;
    float humidity;
} http_msg_t;

// Globais de Controle
QueueHandle_t q_distancia;
QueueHandle_t q_aht10;
QueueHandle_t q_http;
EventGroupHandle_t system_events;

static system_state_t system_state = { .dados_validos = false };


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
// ----------- Controle não bloqueante do LED -----------------------------------
// -----------------------------------------------------------------------------

typedef enum {
    LED_IDLE,
    LED_ERRO_ATIVO,
    LED_OK_ATIVO
} led_state_t;

static led_state_t led_state = LED_IDLE;
static absolute_time_t led_timeout;

void led_clear_all(){
    gpio_put(LED_VERDE, 0);
    gpio_put(LED_AZUL, 0);
    gpio_put(LED_VERMELHO, 0);
}

void setup_leds(){
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    led_clear_all();
}

void led_update(){
    if (led_state == LED_IDLE) return;
    if (absolute_time_diff_us(get_absolute_time(), led_timeout) <= 0){
        led_clear_all();
        led_state = LED_IDLE;
    }
}

void led_erro_geral(){
    if (led_state == LED_ERRO_ATIVO) return;
    led_clear_all();
    gpio_put(LED_VERMELHO, 1);
    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

static inline void led_branco() {
    gpio_put(LED_VERDE, 1);
    gpio_put(LED_AZUL, 1);
    gpio_put(LED_VERMELHO, 1);
}

void led_wifi_pisca_branco() {
    for (int i = 0; i < 3; i++) {
        led_branco();
        vTaskDelay(pdMS_TO_TICKS(80));
        led_clear_all();
        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

void led_erro_sensorDistancia(){
    if (led_state == LED_ERRO_ATIVO) return;
    led_clear_all();
    gpio_put(LED_VERDE, 1);
    gpio_put(LED_VERMELHO, 1);
    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

void led_sem_alcance_sensorDistancia(){
    if (led_state == LED_ERRO_ATIVO) return;
    led_clear_all();
    gpio_put(LED_VERDE, 1);
    gpio_put(LED_AZUL, 1);
    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

void led_disparo_sensorDistancia(){
    if (led_state == LED_OK_ATIVO) return;
    led_clear_all();
    gpio_put(LED_AZUL, 1);
    led_state = LED_OK_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

void led_erro_sensorAHT10(){
    if (led_state == LED_ERRO_ATIVO) return;
    led_clear_all();
    gpio_put(LED_AZUL, 1);
    gpio_put(LED_VERMELHO, 1);
    led_state = LED_ERRO_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}

void led_compra(){
    if (led_state == LED_OK_ATIVO) return;
    led_clear_all();
    gpio_put(LED_VERDE, 1);
    led_state = LED_OK_ATIVO;
    led_timeout = make_timeout_time_ms(500);
}   

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

void buzzer_init(buzzer_t* buzzer, uint pin) {
    buzzer->pin = pin;
    buzzer->slice = pwm_gpio_to_slice_num(pin);
    buzzer->state = BUZZER_IDLE;
    gpio_set_function(pin, GPIO_FUNC_PWM);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096.0f));
    pwm_init(buzzer->slice, &config, true);
    pwm_set_gpio_level(pin, 0);
}

void buzzer_beep(buzzer_t* buzzer, uint duration_ms) {
    if (buzzer->state != BUZZER_IDLE) return;
    pwm_set_gpio_level(buzzer->pin, 2048); 
    buzzer->state = BUZZER_ATIVO;
    buzzer->timeout = make_timeout_time_ms(duration_ms);
}

void buzzer_update(buzzer_t* buzzer) {
    if (buzzer->state == BUZZER_IDLE) return;
    if (absolute_time_diff_us(get_absolute_time(), buzzer->timeout) <= 0) {
        pwm_set_gpio_level(buzzer->pin, 0);
        buzzer->state = BUZZER_IDLE;
    }
}

// -----------------------------------------------------------------------------
// ----------- Funções e Setup do Display OLED ----------------------------------
// -----------------------------------------------------------------------------

void setup_displayOLED(){
    i2c_init(I2C_PORT_DISPLAY, ssd1306_i2c_clock * 1000);
    gpio_set_function(DISPLAY_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DISPLAY_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DISPLAY_SDA_PIN);
    gpio_pull_up(DISPLAY_SCL_PIN);

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
// ----------- Setup de Sensores e Botões --------------------------------------
// -----------------------------------------------------------------------------

void setup_i2c0() { 
    i2c_init(I2C_DIST_AHT10, 100 * 1000);
    gpio_set_function(DIST_AHT10_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(DIST_AHT10_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(DIST_AHT10_SDA_PIN);
    gpio_pull_up(DIST_AHT10_SCL_PIN); 
}

VL53L0X sensorDistancia;
bool setup_sensorDistancia(VL53L0X* sensorDistancia){
    if (!vl53l0x_init(sensorDistancia, I2C_DIST_AHT10, DIST_AHT10_SDA_PIN, DIST_AHT10_SCL_PIN)){
        xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_DIST);
        return false;
    }
    return true;
}

AHT10 sensorAHT10;
bool setup_sensorAHT10(AHT10* sensorAHT10){
    if (!aht10_init(sensorAHT10, I2C_DIST_AHT10, DIST_AHT10_SDA_PIN, DIST_AHT10_SCL_PIN)) {
        xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_AHT10);
        return false;
    }
    return true;
}

void botao_compra_interrupcao(uint gpio, uint32_t events) {

    static absolute_time_t last_irq_time;

    if (gpio != BOTAO_COMPRA_PIN || !(events & GPIO_IRQ_EDGE_FALL)) {
        return;
    }

    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(last_irq_time, now) < 200000) {
        return;
    }
    last_irq_time = now;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 1️⃣ Envia COMPRA imediatamente para HTTP
    http_msg_t http_msg = {
        .type = HTTP_EVT_COMPRA,
        .value_u32 = 1
    };

    xQueueSendFromISR(q_http, &http_msg, &xHigherPriorityTaskWoken);

    // 2️⃣ Apenas para UI (LED / display)
    xEventGroupSetBitsFromISR(
        system_events,
        EVT_COMPRA_REALIZADA,
        &xHigherPriorityTaskWoken
    );

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void setup_botao_compra() {
    gpio_init(BOTAO_COMPRA_PIN);
    gpio_set_dir(BOTAO_COMPRA_PIN, GPIO_IN);
    gpio_pull_up(BOTAO_COMPRA_PIN);

    // Habilita interrupção por borda de descida
    gpio_set_irq_enabled_with_callback(
        BOTAO_COMPRA_PIN,
        GPIO_IRQ_EDGE_FALL,
        true,
        &botao_compra_interrupcao
    );
}

// -----------------------------------------------------------------------------
// ----------- Tarefas FreeRTOS ------------------------------------------------
// -----------------------------------------------------------------------------

// Tarefa Buzzer - Responsável por controlar os beeps do buzzer
void task_buzzer(void *pvParameters) {
    for (;;) {
        xEventGroupWaitBits(
            system_events,
            EVT_BUZZER_BEEP,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );
        buzzer_beep(&buzzerA, 200);
        buzzer_beep(&buzzerB, 200);
        while (buzzerA.state != BUZZER_IDLE || buzzerB.state != BUZZER_IDLE) {
            buzzer_update(&buzzerA);
            buzzer_update(&buzzerB);
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

// Tarefa VL53L0X - Responsável por ler o sensor de distância
void task_vl53l0x(void *pvParameters) {
    vl53_msg_t msg;
    
    for (;;) {
        msg.distance_cm = vl53l0x_read_single_cm(&sensorDistancia);

        // Erro físico do sensor
        if (msg.distance_cm == 0 || msg.distance_cm == 6553) {
            xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_DIST);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        // Envia sempre o último valor (overwrite evita backlog)
        xQueueOverwrite(q_distancia, &msg);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarefa AHT10 - Responsável por ler o sensor de temperatura e umidade
void task_aht10(void *pvParameters) {
    aht10_msg_t msg;
    for (;;) {
        msg.temperature = aht10_get_temperature(&sensorAHT10);
        msg.humidity    = aht10_get_humidity(&sensorAHT10);
        if (msg.temperature <= -1000.0f || msg.humidity < 0.0f) {
            xEventGroupSetBits(system_events, EVT_ERRO_SENSOR_AHT10);
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        xQueueOverwrite(q_aht10, &msg);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Tarefa UI - Responsável por atualizar o display e LEDs conforme eventos
void task_ui(void *pvParameters) {
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
            EVT_UI_UPDATE,
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
        else if (events & EVT_UI_UPDATE) {
            display_info_gerais(
                system_state.distance_cm,
                system_state.temperature,
                system_state.humidity
            );
        }

        // loop de manutenção
        for (int i = 0; i < 5; i++) {
            led_update();
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// -----------------------------------------------------------------------------
// ----------- Tarefa Wi-Fi ----------------------------------------------------
// -----------------------------------------------------------------------------

void task_wifi(void *pvParameters) {
    (void) pvParameters;

    // Inicializa stack Wi-Fi + lwIP (modo FreeRTOS)
    if (cyw43_arch_init()) {
        xEventGroupSetBits(system_events, EVT_WIFI_ERROR);
        // Indica erro geral
        led_erro_geral();
        vTaskDelete(NULL);
    }

    cyw43_arch_enable_sta_mode();

    int result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (result != 0) {
        xEventGroupSetBits(system_events, EVT_WIFI_ERROR);
        // Indica erro geral
        led_erro_geral();
        vTaskDelete(NULL);
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    xEventGroupSetBits(system_events, EVT_WIFI_CONNECTED);

    // A task pode ficar viva apenas para manter o driver ativo
    for (;;) {
        int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

        if (status == CYW43_LINK_UP) {
            xEventGroupSetBits(system_events, EVT_WIFI_CONNECTED);
            xEventGroupClearBits(system_events, EVT_WIFI_ERROR);

            // Pisca branco a cada 10 segundos
            led_wifi_pisca_branco();
            vTaskDelay(pdMS_TO_TICKS(10000));
        } 
        else {
            xEventGroupClearBits(system_events, EVT_WIFI_CONNECTED);
            xEventGroupSetBits(system_events, EVT_WIFI_ERROR);

            // Indica erro geral
            led_erro_geral();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

}

// -----------------------------------------------------------------------------
// ----------- Tarefa Envio HTTP -----------------------------------------------
// -----------------------------------------------------------------------------
void task_http(void *pvParameters) {
    http_msg_t msg;

    for (;;) {

        if (xQueueReceive(q_http, &msg, portMAX_DELAY) == pdPASS) {

            switch (msg.type) {

                case HTTP_EVT_VISITA:
                    printf("[HTTP] Enviar VISITA: %lu\n", msg.value_u32);
                    break;

                case HTTP_EVT_COMPRA:
                    printf("[HTTP] Enviar COMPRA\n");
                    break;

                case HTTP_EVT_TEMPERATURA:
                    printf("[HTTP] Enviar TEMP: %.2f C | HUM: %.2f %%\n",
                           msg.temperature, msg.humidity);
                    break;
            }

            // 🚧 AQUI entra futuramente:
            // http_post("/api/...", json_payload);
        }
    }
}


// -----------------------------------------------------------------------------
// --- Tarefa Principal - Responsável por coordenar leituras e atualizações-----
// -----------------------------------------------------------------------------
void task_main(void *pvParameters) {
    vl53_msg_t msg;
    aht10_msg_t aht_msg;
    static bool sem_alcance_ativo = false;
    static uint8_t visitante_raw_count = 0;
    static bool visitante_presente = false;
    static absolute_time_t last_temp_send;


    // -------------------------------------------------
    // Aguarda Wi-Fi conectar antes de iniciar o sistema
    // -------------------------------------------------
    xEventGroupWaitBits(
        system_events,
        EVT_WIFI_CONNECTED,
        pdFALSE,   // NÃO limpa o bit
        pdTRUE,    // espera exatamente esse bit
        portMAX_DELAY
    );

    // Opcional: feedback visual inicial
    led_wifi_pisca_branco();

    // Inicializa o timer de envio de temperatura
    last_temp_send = make_timeout_time_ms(TEMPO_ENVIO_TEMPERATURA_MS);

    // -------------------------------------------------
    // Loop principal do sistema
    // -------------------------------------------------
    for (;;) {
        // ---------------- VL53L0X ----------------
        if (xQueueReceive(q_distancia, &msg, pdMS_TO_TICKS(20)) == pdPASS) {

            system_state.distance_cm = msg.distance_cm;
            system_state.dados_validos = true;

            if (msg.distance_cm > DIST_MAX_VALIDA) {
                xEventGroupSetBits(system_events, EVT_SEM_ALCANCE);
                sem_alcance_ativo = true;
            } else {
                sem_alcance_ativo = false;
                if (msg.distance_cm <= DIST_VISITANTE_CM) {
                    if (!visitante_presente) {
                        visitante_presente = true;
                        visitante_raw_count++;

                        if (visitante_raw_count >= 2) {
                            http_msg_t http_msg = {
                                .type = HTTP_EVT_VISITA,
                                .value_u32 = 1   // sempre envia 1 visita real
                            };
                            // Enviar mensagem HTTP
                            xQueueSend(q_http, &http_msg, pdMS_TO_TICKS(50));
                            visitante_raw_count = 0; // zera contador
                        }
                        xEventGroupSetBits(system_events, EVT_VISITANTE_DETECT);
                        xEventGroupSetBits(system_events, EVT_BUZZER_BEEP);
                    }
                } else {
                    visitante_presente = false;
                }
            }
        }

        // ---------------- AHT10 ----------------
        if (xQueueReceive(q_aht10, &aht_msg, 0) == pdPASS) {
            system_state.temperature = aht_msg.temperature;
            system_state.humidity    = aht_msg.humidity;
            system_state.dados_validos = true;

        }
        
        // ---------------- ENVIO PERIÓDICO DE TEMPERATURA ----------------
        if (absolute_time_diff_us(get_absolute_time(), last_temp_send) <= 0) {
            http_msg_t http_msg = {
                .type = HTTP_EVT_TEMPERATURA,
                .temperature = system_state.temperature,
                .humidity = system_state.humidity
            };
            xQueueSend(q_http, &http_msg, pdMS_TO_TICKS(50));
            // Reinicia timer
            last_temp_send = make_timeout_time_ms(TEMPO_ENVIO_TEMPERATURA_MS);
        }

        // ---------------- UI ----------------
        if (system_state.dados_validos && !sem_alcance_ativo) {
            xEventGroupSetBits(system_events, EVT_UI_UPDATE);
        }

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

    // Inicialização de Hardware
    setup_leds();
    led_clear_all(); // Garante todos os LEDs desligados inicialmente
    setup_botao_compra();
    setup_displayOLED();
    setup_i2c0();
    buzzer_init(&buzzerA, BUZZER_A_PIN);
    buzzer_init(&buzzerB, BUZZER_B_PIN);

    // Inicialização de Sensores
    setup_sensorDistancia(&sensorDistancia);
    setup_sensorAHT10(&sensorAHT10);

    // Recursos para filas
    q_distancia = xQueueCreate(1, sizeof(vl53_msg_t));
    q_aht10 = xQueueCreate(1, sizeof(aht10_msg_t));
    q_http = xQueueCreate(5, sizeof(http_msg_t));

    // Criação de Tasks
    xTaskCreate(task_vl53l0x, "VL53", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(task_aht10, "AHT10", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_buzzer, "Buzzer", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_ui, "UITask", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_wifi, "WiFi", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(task_http, "HTTP", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_main, "MainTask", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    vTaskStartScheduler();
    while (true) {}
}