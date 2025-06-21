
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

// Configurações de Wi-Fi
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "password"

// Definição dos pinos
#define LED_PIN CYW43_WL_GPIO_LED_PIN
#define LED_RED_PIN 13

#define VRX_PIN 27 // Eixo X
#define VRY_PIN 26 // Eixo Y

// Função para ler valores do ADC (joystick)
uint16_t read_adc(uint gpio) {
    if (gpio == VRX_PIN) {
        adc_select_input(1); // Canal 1 -> GPIO27
    } else {
        adc_select_input(0); // Canal 0 -> GPIO26
    }
    return adc_read();
}

// Função para determinar a direção do joystick
const char* get_direction(uint16_t x, uint16_t y) {
    const uint16_t deadzone = 300; // Zona morta
    const int center = 2048;

    int dx = x - center;
    int dy = y - center;

    if (abs(dx) < deadzone && abs(dy) < deadzone) return "Centro";

    if (dy > deadzone) {
        if (dx > deadzone) return "Nordeste";
        else if (dx < -deadzone) return "Noroeste";
        else return "Norte";
    } else if (dy < -deadzone) {
        if (dx > deadzone) return "Sudeste";
        else if (dx < -deadzone) return "Sudoeste";
        else return "Sul";
    } else {
        if (dx > deadzone) return "Leste";
        else if (dx < -deadzone) return "Oeste";
    }

    return "Desconhecido";
}

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Controle dos LEDs
    if (strstr(request, "GET /on") != NULL) {
        cyw43_arch_gpio_put(LED_PIN, 1);
    } else if (strstr(request, "GET /off") != NULL) {
        cyw43_arch_gpio_put(LED_PIN, 0);
    }

    // Leitura do joystick
    uint16_t x = read_adc(VRX_PIN);
    uint16_t y = read_adc(VRY_PIN);
    const char* direcao = get_direction(x, y);

    // Criação da resposta HTML
    char html[1024];
    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<meta http-equiv=\"refresh\" content=\"1\">\n"
             "<meta charset=\"utf-8\" />\n"
             "<title>Joystick Status</title>\n"
             "<style>\n"
             "body { font-family: Arial; text-align: center; margin-top: 50px; }\n"
             "h1 { font-size: 48px; margin-bottom: 20px; }\n"
             ".data { font-size: 28px; margin: 15px; color: #333; }\n"
             "button { font-size: 24px; margin: 10px; padding: 10px 20px; border-radius: 8px; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Status do Joystick</h1>\n"
             "<p class=\"data\">Posição X: %d</p>\n"
             "<p class=\"data\">Posição Y: %d</p>\n"
             "<p class=\"data\">Direção: %s</p>\n"
             "<form action=\"./\"><button>Atualizar Posições</button></form>\n"
             "</body>\n"
             "</html>\n",
             x, y, direcao);

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    free(request);
    pbuf_free(p);

    return ERR_OK;
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

void setup(){
    stdio_init_all();
    
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);
    
    // Inicialização do ADC
    adc_init();
    adc_gpio_init(VRX_PIN);
    adc_gpio_init(VRY_PIN);
}

int main() {

    setup();

    if (cyw43_arch_init()) {
        printf("Falha ao inicializar Wi-Fi\n");
        return -1;
    }

    cyw43_arch_gpio_put(LED_PIN, 0);
    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        gpio_put(LED_RED_PIN, 1);
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    if (netif_default) {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Criação do servidor
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Erro ao criar servidor TCP\n");
        return -1;
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao associar porta 80\n");
        return -1;
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    printf("Servidor ouvindo na porta 80\n");

    while (true) {
        cyw43_arch_poll();
    }

    cyw43_arch_deinit();
    return 0;
}
