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

// Parâmetros Lógicos
#define DIST_MAX_VALIDA 800

// -----------------------------------------------------------------------------
// ----------- Configuração Wi-Fi -----------------------------------------------
// -----------------------------------------------------------------------------

#define WIFI_SSID       "SEU_SSID_WIFI"
#define WIFI_PASSWORD   "SUA_SENHA_WIFI"

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

typedef struct {
    uint16_t distance_cm;
    float temperature;
    float humidity;
    bool dados_validos;
} system_state_t;

typedef struct {
    uint16_t distance_cm;
} vl53_msg_t;

typedef struct {
    float temperature;
    float humidity;
} aht10_msg_t;

// Globais de Controle
QueueHandle_t q_distancia;
QueueHandle_t q_aht10;
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
    // codigo removido para compactacao
}

void led_sem_alcance_sensorDistancia(){
    // codigo removido para compactacao
}

void led_disparo_sensorDistancia(){
    // codigo removido para compactacao
}

void led_erro_sensorAHT10(){
    // codigo removido para compactacao
}

void led_compra(){
    // codigo removido para compactacao
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
    // codigo removido para compactacao
}

void buzzer_beep(buzzer_t* buzzer, uint duration_ms) {
    // codigo removido para compactacao
}

void buzzer_update(buzzer_t* buzzer) {
   // codigo removido para compactacao
}

void setup_displayOLED(){
    // codigo removido para compactacao
}

void display_info_gerais(uint16_t distance_cm, float temperature, float humidity){
    // codigo removido para compactacao
}

void display_erro_sensorDistancia(){
   // codigo removido para compactacao
}

void display_sem_alcance_sensorDistancia(){
    // codigo removido para compactacao
}

void display_disparo_sensorDistancia(){
    // codigo removido para compactacao
}

void display_erro_sensorAHT10(){
    // codigo removido para compactacao
}


void compra_display(){
   // codigo removido para compactacao
}

void setup_i2c0() { 
    // codigo removido para compactacao
}

VL53L0X sensorDistancia;
bool setup_sensorDistancia(VL53L0X* sensorDistancia){
    // codigo removido para compactacao
}

AHT10 sensorAHT10;
bool setup_sensorAHT10(AHT10* sensorAHT10){
    // codigo removido para compactacao
}

void setup_botao_compra(){
    // codigo removido para compactacao
}

void compra_registrada() {
   // codigo removido para compactacao
}

// Tarefa Buzzer - Responsável por controlar os beeps do buzzer
void task_buzzer(void *pvParameters) {
   // codigo removido para compactacao
}

// Tarefa VL53L0X - Responsável por ler o sensor de distância
void task_vl53l0x(void *pvParameters) {
    // codigo removido para compactacao
}

// Tarefa AHT10 - Responsável por ler o sensor de temperatura e umidade
void task_aht10(void *pvParameters) {
    // codigo removido para compactacao
}

// Tarefa UI - Responsável por atualizar o display e LEDs conforme eventos
void task_ui(void *pvParameters) {
    // codigo removido para compactacao
}

// -----------------------------------------------------------------------------
// ----------- Tarefa Wi-Fi -----------------------------------------------------
// -----------------------------------------------------------------------------

void task_wifi(void *pvParameters) {
    (void) pvParameters;

    // Inicializa stack Wi-Fi + lwIP (modo FreeRTOS)
    if (cyw43_arch_init()) {
        printf("Wi-Fi: falha ao inicializar cyw43\n");
        xEventGroupSetBits(system_events, EVT_WIFI_ERROR);
        vTaskDelete(NULL);
    }

    cyw43_arch_enable_sta_mode();
    printf("Wi-Fi: conectando...\n");

    int result = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID,
        WIFI_PASSWORD,
        CYW43_AUTH_WPA2_AES_PSK,
        30000
    );

    if (result != 0) {
        printf("Wi-Fi: falha ao conectar (%d)\n", result);
        xEventGroupSetBits(system_events, EVT_WIFI_ERROR);
        vTaskDelete(NULL);
    }

    printf("Wi-Fi: conectado com sucesso!\n");
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
// --- Tarefa Principal - Responsável por coordenar leituras e atualizações-----
// -----------------------------------------------------------------------------
void task_main(void *pvParameters) {
    vl53_msg_t msg;
    static bool sem_alcance_ativo = false;

    aht10_msg_t aht_msg;

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
                if (msg.distance_cm <= 50) {
                    xEventGroupSetBits(system_events, EVT_VISITANTE_DETECT);
                    xEventGroupSetBits(system_events, EVT_BUZZER_BEEP);
                }
            }
        }

        // ---------------- AHT10 ---------------------------
        if (xQueueReceive(q_aht10, &aht_msg, 0) == pdPASS) {
            system_state.temperature = aht_msg.temperature;
            system_state.humidity    = aht_msg.humidity;
            system_state.dados_validos = true;
        }

        // Compra registrada (botão)
        compra_registrada();

        // SÓ envia atualização comum se NÃO estiver fora de alcance
        if (system_state.dados_validos && !sem_alcance_ativo) {
            xEventGroupSetBits(system_events, EVT_UI_UPDATE);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(){
    stdio_init_all();
    system_events = xEventGroupCreate();

    if (system_events == NULL) {
        xEventGroupSetBits(system_events, EVT_ERRO_GERAL);
    }
    setup_leds();
    led_clear_all();
    setup_botao_compra();
    setup_displayOLED();
    setup_i2c0();
    buzzer_init(&buzzerA, BUZZER_A_PIN);
    buzzer_init(&buzzerB, BUZZER_B_PIN);
    setup_sensorDistancia(&sensorDistancia);
    setup_sensorAHT10(&sensorAHT10);

    q_distancia = xQueueCreate(1, sizeof(vl53_msg_t));
    q_aht10 = xQueueCreate(1, sizeof(aht10_msg_t));

    xTaskCreate(task_vl53l0x, "VL53", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(task_aht10, "AHT10", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_buzzer, "Buzzer", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_ui, "UITask", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_wifi, "WiFi", 2048, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(task_main, "MainTask", 2048, NULL, tskIDLE_PRIORITY + 1, NULL);
    
    vTaskStartScheduler();
    while (true) {}
}