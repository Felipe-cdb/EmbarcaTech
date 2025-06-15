#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"

#define LED_PIN_RED 13 // Definindo Led Vermelho
#define BUTTON_A 5 // Definindo botão A

// status de led e botão
int statusButtonA = 0;
int statusLed = 0;

SemaphoreHandle_t xButtonSemaphore;
TaskHandle_t ledTaskHandle; // Declare um manipulador de tarefa para o LED

void vReadingButtonTask(void *pvParamters) {
    for (;;) {
        printf("Leitura de botão \n");
        if (gpio_get(BUTTON_A) == 0) { // Botão pressionado 
            statusButtonA = 1; 
        } else {
            statusButtonA = 0;      
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void buttonISR(uint gpio, uint32_t events){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void vButtonProcessTask(void *pvParameters) {
    for (;;) {
        printf("Processando botão...\n");
        if (statusButtonA == 1) {
            statusLed = 1;
            xTaskNotifyGive(ledTaskHandle); // Notifica a tarefa do LED
        }
        statusButtonA = 0;
        vTaskDelay(pdMS_TO_TICKS(10)); // controle de fluxo
    }
}

void vLedTask(void *pvParameters) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        gpio_put(LED_PIN_RED, statusLed);
        printf("LED ON\n");
        vTaskDelay(pdMS_TO_TICKS(100));
        statusLed = 0;
        gpio_put(LED_PIN_RED, statusLed);
        printf("LED OFF\n");
    }
}

void setup(void){
    stdio_init_all();
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
}

int main() {

    setup();

    xButtonSemaphore = xSemaphoreCreateBinary();

    if (xButtonSemaphore != NULL) {
        gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, buttonISR);

        xTaskCreate(vReadingButtonTask, "Leitura de Botão - Tarefa 1", 128, NULL, 1, NULL);
        xTaskCreate(vButtonProcessTask, "Processando Botão - Tarefa 2", 128, NULL, 2, NULL);
        xTaskCreate(vLedTask, "Aciona o LED - Tarefa 3", 128, NULL, 3, &ledTaskHandle);

        vTaskStartScheduler();
    }

    while (1);
    return 0;
}