#include "main.h"
#define PUB_DELAY_MS 1000 // Tempo de 1s entre publicações no servidor MQTT
static uint32_t counter = 0;
static uint32_t last_time = 0;
err_t mqtt_test_connect(MQTT_CLIENT_T *state);

void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    MQTT_CLIENT_T *state = (MQTT_CLIENT_T*)callback_arg;
    if (ipaddr) {
        state->remote_addr = *ipaddr;
        DEBUG_printf("DNS resolved: %s\n", ip4addr_ntoa(ipaddr));
    } else {
        DEBUG_printf("DNS resolution failed.\n");
    }
}

void run_dns_lookup(MQTT_CLIENT_T *state) {
    DEBUG_printf("Running DNS lookup for %s...\n", MQTT_SERVER_HOST);
    if (dns_gethostbyname(MQTT_SERVER_HOST, &(state->remote_addr), dns_found, state) == ERR_INPROGRESS) {
        while (state->remote_addr.addr == 0) {
            cyw43_arch_poll();
            sleep_ms(10);
        }
    }
}

static void mqtt_pub_start_cb(void *arg, const char *topic, u32_t tot_len) {
    DEBUG_printf("Incoming message on topic: %s\n", topic);
}

// Callback para receber dados do tópico
static void mqtt_pub_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    // Preparar área de renderização para o display
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);

    // Processar a mensagem recebida
    char buffer[BUFFER_SIZE];
    if (len < BUFFER_SIZE) {
        memcpy(buffer, data, len);
        buffer[len] = '\0';
        DEBUG_printf("Message received: %s\n", buffer);

        // Comando "acender"
        if (strstr(buffer, "\"msg\": \"acender\"") != NULL) {
            pwm_led(LED_PIN_B, 1000);  // Acende o LED Azul
            char *text[] = {
                "...............",
                "...............",
                "...Enviando....",
                "....comando....",
                "...............",
                "     Acender   ",
                "    LED Azul   ",
                "...............",
            };
            int y = 0;
            for (int i = 0; i < sizeof(text) / sizeof(text[0]); i++) {
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 8;  // Incrementa para a próxima linha
            }
            render_on_display(ssd, &frame_area);
        } 
        
        // Comando "apagar"
        else if (strstr(buffer, "\"msg\": \"apagar\"") != NULL) {
            pwm_led(LED_PIN_B, 0);  // Apaga o LED Azul
            char *text[] = {
                "...............",
                "...............",
                "...Enviando....",
                "....comando....",
                "...............",
                "     Apagar    ",
                "    LED Azul   ",
                "...............",
            };
            int y = 0;
            for (int i = 0; i < sizeof(text) / sizeof(text[0]); i++) {
                ssd1306_draw_string(ssd, 5, y, text[i]);
                y += 8;  // Incrementa para a próxima linha
            }
            render_on_display(ssd, &frame_area);
        } 
        
        // Comando "som"
        else if (strstr(buffer, "\"msg\": \"som\"") != NULL) {
            pwm_set_gpio_level(BUZZER_PIN, 400);  // Liga o buzzer
            sleep_ms(500);                        // Toca por 0,5s
            pwm_set_gpio_level(BUZZER_PIN, 0);    // Desliga o buzzer
        }
    } else {
        DEBUG_printf("Message too large, discarding.\n");
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    if (status == MQTT_CONNECT_ACCEPTED) {
        DEBUG_printf("MQTT connected.\n");
        pwm_led(LED_PIN_G, 1000); // Liga o LED Verde
        sleep_ms(2000); // Espera 1 segundo para que o LED verde fique aceso
        pwm_led(LED_PIN_G, 0); // Desliga o LED Verde após o delay
    } else {
        DEBUG_printf("MQTT connection failed: %d\n", status);
        pwm_led(LED_PIN_R, 1000); // Liga o LED Vermelho
        sleep_ms(2000); // Espera 1 segundo para que o LED vermelho fique aceso
        pwm_led(LED_PIN_R, 0); // Desliga o LED Vermelho após o delay
    }
}

void mqtt_pub_request_cb(void *arg, err_t err) {
    DEBUG_printf("Publish request status: %d\n", err);
}

void mqtt_sub_request_cb(void *arg, err_t err) {
    DEBUG_printf("Subscription request status: %d\n", err);
}

err_t mqtt_test_publish(MQTT_CLIENT_T *state) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // Preparar área de renderização para o display
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);


    if (now - last_time >= PUB_DELAY_MS)
    {
        last_time = now;
        char buffer[BUFFER_SIZE];
        printf("linha 148: %d\n", (now - last_time));
        counter += 1;

        // status do botão de alarme
        if (gpio_get(BUTTON6_PIN) == 1) {
            printf("Botão de alarme solto\n");
            snprintf(buffer, BUFFER_SIZE, "Botão de alarme solto !");
            mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), 0, 0, mqtt_pub_request_cb, state);
        } else {
            printf("Botão de alarme pressionado\n");
            snprintf(buffer, BUFFER_SIZE, "Botão de alarme pressionado !");
            mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), 0, 0, mqtt_pub_request_cb, state);
        }

        // status do botão de LED
        if (gpio_get(BUTTON5_PIN) == 1) {
            printf("Botão LED solto\n");
            snprintf(buffer, BUFFER_SIZE, "Botão LED solto !");
            mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), 0, 0, mqtt_pub_request_cb, state);
            pwm_led(LED_PIN_G, 0); // Mantem LED Verde desligado

        } else {
            printf("Botão Distancia pressionado\n");
            snprintf(buffer, BUFFER_SIZE, "Botão Distancia pressionado !");
            mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), 0, 0, mqtt_pub_request_cb, state);
            pwm_led(LED_PIN_G, 1000); // Liga LED Verde

            // Dados do sensor de distância VL53L0X
            VL53L0X sensor;
            if (!vl53l0x_init(&sensor, i2c0, 0x29)) {
                printf("Falha ao inicializar o sensor VL53L0X\n");
                while (1) tight_loop_contents();
            }
            printf("Sensor iniciado com sucesso!\n");
            
            int16_t distancia_mm = vl53l0x_read_distance(&sensor); // Lê a distância em milímetros
            float distancia_cm = distancia_mm / 10.0f; // Converte mm para cm
            // Verifica se a leitura é válida
            if (distancia_mm >= 0 && distancia_mm < 4096) {
                snprintf(buffer, BUFFER_SIZE, "Distância: %.1f cm\n", distancia_cm);
                mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), 0, 0, mqtt_pub_request_cb, state);
                printf("Distância: %.1f cm\n", distancia_cm);
                
                char valor_distancia[20];  // string para armazenar o valor formatado
                snprintf(valor_distancia, sizeof(valor_distancia), "%.1f cm", distancia_cm);
                char *text[] = {
                    "...............",
                    "...............",
                    "...Distancia...",     // título
                    valor_distancia,       // aqui o valor + "cm"
                    "...............",
                    "...............",
                };
                int y = 0;
                for (int i = 0; i < sizeof(text) / sizeof(text[0]); i++) {
                    ssd1306_draw_string(ssd, 5, y, text[i]);
                    y += 8;  // Incrementa para a próxima linha
                }
                render_on_display(ssd, &frame_area);
            } else {
                printf("⚠️ Timeout ou leitura inválida!\n");
                char *text[] = {
                    "...............",
                    "...............",
                    "....Sensor.....",
                    "...Distancia...",
                    "...............",
                    "    leitura    ",
                    "   inválida    ",
                    "...............",
                };
                int y = 0;
                for (int i = 0; i < sizeof(text) / sizeof(text[0]); i++) {
                    ssd1306_draw_string(ssd, 5, y, text[i]);
                    y += 8;  // Incrementa para a próxima linha
                }
                render_on_display(ssd, &frame_area);
            }
        }

        if (alarme == true)
        {   
            pwm_set_gpio_level(BUZZER_PIN, 500); // Som contínuo (50% duty)
            pwm_led(LED_PIN_G, 500);
            pwm_led(LED_PIN_R, 800);
            snprintf(buffer, BUFFER_SIZE, "*ALARME DE VAZAMENTO DE GAS*");
            mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), 0, 0, mqtt_pub_request_cb, state);
        }else {
            pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga buzzer
            pwm_led(LED_PIN_R, 0);
        }
        return mqtt_publish(state->mqtt_client, "pico_w/test", buffer, strlen(buffer), 0, 0, mqtt_pub_request_cb, state);
    }
    else
    {
        return 0;
    }
}

err_t mqtt_test_connect(MQTT_CLIENT_T *state) {
    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = "PicoW";
    return mqtt_client_connect(state->mqtt_client, &(state->remote_addr), MQTT_SERVER_PORT, mqtt_connection_cb, state, &ci);
}

void mqtt_run_test(MQTT_CLIENT_T *state) {
    state->mqtt_client = mqtt_client_new();
    if (!state->mqtt_client) {
        DEBUG_printf("Failed to create MQTT client\n");
        return;
    }

    if (mqtt_test_connect(state) == ERR_OK) {
        while (!mqtt_client_is_connected(state->mqtt_client)) {
            cyw43_arch_poll();  // Espera conexão
            sleep_ms(100);
        }

        // Só após a conexão confirmada:
        mqtt_set_inpub_callback(state->mqtt_client, mqtt_pub_start_cb, mqtt_pub_data_cb, NULL);
        mqtt_sub_unsub(state->mqtt_client, "pico_w/recv", 0, mqtt_sub_request_cb, NULL, 1);

        while (1) {
            cyw43_arch_poll();
            if (mqtt_client_is_connected(state->mqtt_client)) {
                mqtt_test_publish(state);
                static uint64_t last_time_pwm = 0;
                uint64_t current_time = to_ms_since_boot(get_absolute_time());
                if (current_time - last_time_pwm >= FADE_STEP_DELAY) // a cada 100ms vai aumentar o brilho em 400
                {
                    update_pwm(LED_PIN_R); // Atualizar o brilho do LED
                    last_time_pwm = current_time;
                }
                sleep_ms(50);
            } else {
                DEBUG_printf("Reconnecting...\n");
                sleep_ms(250);
                mqtt_test_connect(state);
            }
        }
    }
}