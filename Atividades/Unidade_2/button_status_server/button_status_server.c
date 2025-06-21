#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hardware/adc.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

// Configurações de Wi-Fi
#define WIFI_SSID "WAVE-NEGUINHA_2G"
#define WIFI_PASSWORD "efg346708"

// Pinos
#define LED_PIN CYW43_WL_GPIO_LED_PIN
#define LED_GREEN_PIN 11
#define LED_RED_PIN 13
#define BUTTON_A 5

bool buttonA_state = false;
volatile float global_temperature = 0.0f;
volatile bool global_button_state = false;

void vButtonReadTask(void *pvParamters)
{
    for (;;)
    {
        bool btn_pressed = !gpio_get(BUTTON_A);
        if (btn_pressed)
        {
            gpio_put(LED_GREEN_PIN, true);
            buttonA_state = true;
            while (!gpio_get(BUTTON_A))
            {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            gpio_put(LED_GREEN_PIN, false);
            buttonA_state = false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vSendServerTask(void *pvParamters)
{
    for (;;)
    {
        printf("Enviando dados para o servidor ...\n");
        printf(" [STATUS: %s]\n", buttonA_state ? "pressionado" : "solto");

        // Leitura da temperatura interna
        adc_select_input(4);
        uint16_t raw_value = adc_read();
        const float conversion_factor = 3.3f / (1 << 12);
        global_temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
        global_button_state = buttonA_state;

        printf(" [TEMP: %.2f°C]\n", global_temperature);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vWiFiSetupTask(void *pvParameters)
{
    if (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        vTaskDelete(NULL);
    }

    cyw43_arch_gpio_put(LED_PIN, 0);
    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Erro ao conectar no Wi-Fi\n");
        gpio_put(LED_RED_PIN, 1);
        vTaskDelete(NULL);
    }

    printf("Conectado ao Wi-Fi!\n");
    cyw43_arch_gpio_put(LED_PIN, 1);

    if (netif_default)
    {
        printf("IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    vTaskDelete(NULL);
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    if (strstr(request, "GET /on") != NULL)
    {
        cyw43_arch_gpio_put(LED_PIN, 1);
    }
    else if (strstr(request, "GET /off") != NULL)
    {
        cyw43_arch_gpio_put(LED_PIN, 0);
    }

    char html[1024];
    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<meta charset=\"utf-8\" />\n"
             "<title>Botão Server</title>\n"
             "<style>\n"
             "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
             "h1 { font-size: 64px; margin-bottom: 30px; }\n"
             "button { font-size: 36px; margin: 10px; padding: 20px 40px; border-radius: 10px; }\n"
             ".temperature { font-size: 48px; margin-top: 30px; color: #333; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Status do Botão</h1>\n"
             "<form action=\"./\"><button>Atualizar Status</button></form>\n"
             "<p class=\"temperature\" id=\"temp\">Temperatura: %.2f °C</p>\n"
             "<p class=\"temperature\" id=\"botao\">Status do Botão: %s</p>\n"
             "<script>\n"
             "function atualizarStatus() {\n"
             "  fetch('./?' + Date.now())\n"
             ".then(response => response.json())\n"
             ".then(data => {\n"
             "document.getElementById('temp').textContent = 'Temperatura: ' + data.temperatura.toFixed(2) + ' °C';\n"
             "document.getElementById('botao').textContent = 'Status do Botão: ' + data.botao;\n"
             "})\n"
             ".catch(error => console.error('Erro ao buscar status:', error));\n"
             "}\n"
             "setInterval(atualizarStatus, 1000);\n"
             "atualizarStatus();\n"
             "</script>\n"
             "</body>\n"
             "</html>\n",
             global_temperature,
             global_button_state ? "pressionado" : "solto");

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    free(request);
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

void vTCPServerTask(void *pvParameters)
{
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        vTaskDelete(NULL);
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        vTaskDelete(NULL);
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    for (;;)
    {
        cyw43_arch_poll(); // essencial para Wi-Fi
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup()
{
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);

    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    stdio_init_all(); // Para printf USB

    // Inicializa ADC e sensor de temperatura aqui também
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

int main()
{
    setup();

    xTaskCreate(vWiFiSetupTask, "WiFi Setup", 1024, NULL, 3, NULL);
    xTaskCreate(vTCPServerTask, "Servidor TCP", 1024, NULL, 3, NULL);
    xTaskCreate(vButtonReadTask, "Leitura Botao", 256, NULL, 1, NULL);
    xTaskCreate(vSendServerTask, "Envia Dados", 256, NULL, 2, NULL);

    vTaskStartScheduler();

    while (true)
    {
    } // não sai do loop
    return 0;
}
