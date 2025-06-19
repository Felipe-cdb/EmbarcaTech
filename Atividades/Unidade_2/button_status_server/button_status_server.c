#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_GREEN_PIN 11 // GPIO11 - LED verde
#define BUTTON_A 5 // Definindo botão A

// Variável global para controlar o estado do botão A
bool buttonA_state = false;

// Leitura do botão
void vButtonReadTask(void *pvParamters) {
    for (;;) {
        bool btn_pressed = !gpio_get(BUTTON_A); // Ativo em nível baixo (pull-up)
        
        if (btn_pressed) {
            gpio_put(LED_GREEN_PIN, true);
            buttonA_state = true;
            // printf("Botão pressionado\n");

            // Aguarda até o botão ser solto
            while (!gpio_get(BUTTON_A)) {
                vTaskDelay(pdMS_TO_TICKS(20));
            }

            gpio_put(LED_GREEN_PIN, false);
            buttonA_state = false;
            // printf("Botão solto\n");
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Evita leitura constante
    }
}

// Simulação do envio ao servidor
void vServerTask(void *pvParamters) {
    for (;;) {
        printf("Enviando dados para o servidor ...\n");

        if (buttonA_state) {
            printf(" [STATUS: Botão pressionado]\n");
        } else {
            printf(" [STATUS: Botão solto]\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Aguarda 1 segundo
    }
}

void setup() {

    // Configura o LED
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);  // Inicializa o led apagado

    // Configura o botão com pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);  // Habilita o resistor pull-up interno
    
    stdio_init_all();

    printf("Programa Iniciado!\n");
}

int main()
{
    setup();

    xTaskCreate(vButtonReadTask, "Leitura de Botão", 128, NULL, 2, NULL);
    xTaskCreate(vServerTask, "Envia dados ao servidor", 128, NULL, 1, NULL);
    
    vTaskStartScheduler();
    
    for (;;){
        // Initialise the Wi-Fi chip
        if (cyw43_arch_init()) {
            printf("Wi-Fi init failed\n");
            return -1;
        }

        // Liga led interno, usado para mostrar que o programa foi iniciado
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }
    return 0; // Retorne 0
}