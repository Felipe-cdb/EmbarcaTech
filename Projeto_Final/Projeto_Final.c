#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Sensores
#include "vl53l0x/vl53l0x.h"
#include "aht10/aht10.h"

// Display
#include "ssd1306/ssd1306.h"

// SD Card
#include "bitdoglab_sdcard/sd_card.h"

/* ===================== DEFINIÇÕES DE PINOS ===================== */

// LEDs
#define LED_ENTRADA     13
#define LED_COMPRA      11
#define LED_MATRIX_ERR  7

// Botão
#define BUTTON_COMPRA   5

// Buzzer
#define BUZZER_PIN      21

/* ===================== I2C ===================== */

// AHT10
#define I2C_PORT_AHT10  i2c0
#define SDA_PIN_AHT10   0
#define SCL_PIN_AHT10   1

// VL53L0X
#define I2C_PORT_DIST   i2c1
#define SDA_PIN_DIST    2
#define SCL_PIN_DIST    3

// Display
#define I2C_PORT_DISPLAY i2c1
#define SDA_PIN_DISPLAY  14
#define SCL_PIN_DISPLAY  15

/* ===================== PARÂMETROS ===================== */

#define DISTANCIA_PORTA_CM 200
#define LIMIAR_PASSAGEM_CM 120

/* ===================== OBJETOS ===================== */

VL53L0X sensor_dist;
AHT10   sensor_temp;

/* ===================== DISPLAY ===================== */

uint8_t ssd_buffer[ssd1306_buffer_length];
struct render_area display_area;

/* ===================== CONTADORES ===================== */

volatile uint32_t visitas_dia = 0;
volatile uint32_t vendas_dia  = 0;
volatile uint32_t passagens   = 0;

/* ===================== FILAS ===================== */

QueueHandle_t q_display;
QueueHandle_t q_sd;

/* ===================== TIPOS ===================== */

typedef enum {
    EVENTO_ENTRADA,
    EVENTO_COMPRA,
    EVENTO_ERRO_SENSOR
} evento_t;

/* ===================== BUZZER ===================== */

void buzzer_beep(void) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 7.8f);
    pwm_init(slice, &cfg, true);

    pwm_set_gpio_level(BUZZER_PIN, 400);
    sleep_ms(500);
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

/* ===================== TASK SENSOR DISTÂNCIA ===================== */

void TaskDistanceSensor(void *p) {
    uint16_t last_distance = DISTANCIA_PORTA_CM + 50;

    for (;;) {
        uint16_t dist = vl53l0x_read_continuous_cm(&sensor_dist);

        printf("Distancia lida: %d cm\n", dist);

        // Detecta alguém passando
        if (dist != 6553 &&
            dist < DISTANCIA_PORTA_CM &&
            last_distance >= DISTANCIA_PORTA_CM) {

            printf(">>> PASSAGEM DETECTADA <<<\n");

            gpio_put(LED_ENTRADA, 1);
            buzzer_beep();
            gpio_put(LED_ENTRADA, 0);

            passagens++;

            if (passagens % 2 == 0) {
                visitas_dia++;

                evento_t ev = EVENTO_ENTRADA;
                xQueueSend(q_display, &ev, 0);
                xQueueSend(q_sd, &ev, 0);
            }
        }

        last_distance = dist;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


/* ===================== TASK BOTÃO COMPRA ===================== */

void TaskButtonSales(void *p) {
    for (;;) {
        if (!gpio_get(BUTTON_COMPRA)) {
            vendas_dia++;

            gpio_put(LED_COMPRA, 1);
            sleep_ms(200);
            gpio_put(LED_COMPRA, 0);

            evento_t ev = EVENTO_COMPRA;
            xQueueSend(q_display, &ev, 0);
            xQueueSend(q_sd, &ev, 0);

            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ===================== TASK DISPLAY ===================== */

void TaskDisplay(void *p) {
    evento_t ev;

    for (;;) {
        if (xQueueReceive(q_display, &ev, portMAX_DELAY)) {

            memset(ssd_buffer, 0, ssd1306_buffer_length);

            char l1[32], l2[32], l3[32];

            sprintf(l1, "Visitas: %lu", visitas_dia);
            sprintf(l2, "Vendas : %lu", vendas_dia);

            if (ev == EVENTO_ENTRADA)
                sprintf(l3, "Passagem cliente");
            else if (ev == EVENTO_COMPRA)
                sprintf(l3, "Compra registrada");
            else
                sprintf(l3, "ERRO SENSOR!");

            char *text[] = {
                "Dashboard Diario",
                l1,
                l2,
                "----------------",
                l3,
                "EmbarcaTech IoT"
            };

            int y = 0;
            for (uint i = 0; i < count_of(text); i++) {
                ssd1306_draw_string(ssd_buffer, 5, y, text[i]);
                y += 8;
            }

            render_on_display(ssd_buffer, &display_area);
        }
    }
}

/* ===================== TASK SD CARD ===================== */

void TaskSDLogger(void *p) {
    evento_t ev;
    char buffer[128];

    for (;;) {
        if (xQueueReceive(q_sd, &ev, portMAX_DELAY)) {
            sprintf(buffer,
                "Visitas:%lu Vendas:%lu Evento:%d\n",
                visitas_dia, vendas_dia, ev);

            sd_card_write_text("dados_dashboard.txt", buffer);
        }
    }
}

/* ===================== TASK MONITORAMENTO ===================== */

void TaskHealthMonitor(void *p) {
    for (;;) {
        if (vl53l0x_read_single_cm(&sensor_dist) == 6553 ||
            aht10_get_temperature(&sensor_temp) < -900) {

            gpio_put(LED_MATRIX_ERR, 1);

            evento_t ev = EVENTO_ERRO_SENSOR;
            xQueueSend(q_display, &ev, 0);
        } else {
            gpio_put(LED_MATRIX_ERR, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ===================== MAIN ===================== */

int main() {
    stdio_init_all();
    sleep_ms(100);

    /* -------- DISPLAY -------- */
    i2c_init(I2C_PORT_DISPLAY, ssd1306_i2c_clock * 1000);
    gpio_set_function(SDA_PIN_DISPLAY, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_DISPLAY, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_DISPLAY);
    gpio_pull_up(SCL_PIN_DISPLAY);

    ssd1306_init();

    memset(ssd_buffer, 0, ssd1306_buffer_length);

    char *boot_text[] = {
        "    Iniciando  ",
        "     sistema   ",
        "        ______ ",
        "       | _  _ |",
        "       |(.)(.)|",
        "       |  __  |",
        "       |      |",
        "       |      |",
    };

    int y = 0;
    for (uint i = 0; i < count_of(boot_text); i++) {
        ssd1306_draw_string(ssd_buffer, 5, y, boot_text[i]);
        y += 8;
    }

    render_on_display(ssd_buffer, &display_area);
    sleep_ms(2000);


    /* -------- GPIO -------- */
    gpio_init(LED_ENTRADA);
    gpio_init(LED_COMPRA);
    gpio_init(LED_MATRIX_ERR);

    gpio_set_dir(LED_ENTRADA, GPIO_OUT);
    gpio_set_dir(LED_COMPRA, GPIO_OUT);
    gpio_set_dir(LED_MATRIX_ERR, GPIO_OUT);

    gpio_init(BUTTON_COMPRA);
    gpio_set_dir(BUTTON_COMPRA, GPIO_IN);
    gpio_pull_up(BUTTON_COMPRA);

    /* -------- SENSORES -------- */
    vl53l0x_init(&sensor_dist, I2C_PORT_DIST, SDA_PIN_DIST, SCL_PIN_DIST);
    vl53l0x_start_continuous(&sensor_dist, 0);

    aht10_init(&sensor_temp, I2C_PORT_AHT10, SDA_PIN_AHT10, SCL_PIN_AHT10);

    /* -------- SD -------- */
    sd_card_init();

    /* -------- FILAS -------- */
    q_display = xQueueCreate(5, sizeof(evento_t));
    q_sd      = xQueueCreate(5, sizeof(evento_t));

    /* -------- TASKS -------- */
    xTaskCreate(TaskDistanceSensor, "DIST", 256, NULL, 3, NULL);
    xTaskCreate(TaskButtonSales,    "BTN",  256, NULL, 2, NULL);
    xTaskCreate(TaskDisplay,        "DSP",  512, NULL, 1, NULL);
    xTaskCreate(TaskSDLogger,       "SD",   512, NULL, 1, NULL);
    xTaskCreate(TaskHealthMonitor,  "HLT",  256, NULL, 1, NULL);

    vTaskStartScheduler();
    while (1);
}
