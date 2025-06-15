#include "pico/stdlib.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN_RED 13 // Definindo Led Vermelho
#define BUTTON_A 5 // Definindo botão A

// status de led e botão
int statusButtonA = 0;
int statusLed = 0;

TaskHandle_t ledTaskHandle; // Declare um manipulador de tarefa para o LED

void vButtonReadTask(void *pvParamters) {
    for (;;) {

        statusButtonA = gpio_get(BUTTON_A);
        printf("status botão \n");

        if (statusButtonA==1){
            printf("leitura botão \n");
        }        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vButtonProcessTask(void *pvParamters) {
    for (;;) {
        if (statusButtonA == 1) {
            xTaskNotifyGive(ledTaskHandle); // Use o manipulador do LED
            printf("processado botão\n");
        }
        statusButtonA = 0;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vLedTask(void *pvParamters) {
    for (;;) {
        if ((ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) == true){
            statusLed = 1; // Defina o status do LED
            gpio_put(LED_PIN_RED, statusLed); // Corrija para gpio_put
            printf("acendeu o led\n");
        } else {
            statusLed = 0;
            gpio_put(LED_PIN_RED, statusLed); 
        }
    }
}

void setup() {
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    stdio_init_all();
}

int main(void) {
    setup();

    xTaskCreate(vButtonReadTask, "Leitura de Botão - tarefa 1", 128, NULL, 1, NULL);
    xTaskCreate(vButtonProcessTask, "Aciona Botão - tarefa 2", 128, NULL, 2, NULL);
    xTaskCreate(vLedTask, "Aciona Led - tarefa 3", 128, NULL, 3, &ledTaskHandle); // Use o manipulador do LED

    vTaskStartScheduler();
    for (;;){
        /* code */
    }
    return 0; // Retorne 0
}