#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#define LED_GREEN_PIN 11 // GPIO11 - LED verde
#define BUTTON_A 5 // Definindo botão A

// Variável global para controlar o estado do LED Verde
bool led_state = false;
// Variável global para controlar o estado do botão A
bool buttonA_state = false;

int main()
{
    stdio_init_all();

    // Configura o LED
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);  // Inicializa o led apagado

    // Configura o botão com pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);  // Habilita o resistor pull-up interno


    // Initialise the Wi-Fi chip
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    // Liga led interno, usado para mostrar que o programa foi iniciado
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    while (true) {
        printf("Programa Iniciado!\n");
        // sleep_ms(1000); // 1000ms = 1 segundo

        // Verifica o estado do botão (ativo em nível lógico 0 - lógica invertida devido ao pullup)
        if (!gpio_get(BUTTON_A)){  // gpio_get(BUTTON_A) retorna 0 quando pressionado
            sleep_ms(10); // Delay para debounce
            // Confirma se ainda está pressionado após o debounce
            if (!gpio_get(BUTTON_A)) {
                printf("Botão pressionado\n");
                // Aguarda até soltar o botão
                while (!gpio_get(BUTTON_A)) {
                    sleep_ms(1);
                }

                // Alterna estado
                buttonA_state = !buttonA_state;
                led_state = !led_state;
                gpio_put(LED_GREEN_PIN, led_state);
            }
        } 
        printf("Estado atual do botão: %s\n", buttonA_state ? "ligado" : "desligado");
        sleep_ms(10); // Pequeno delay para não sobrecarregar o loop
    }
}