#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"


#define SERVO_PIN     0   // GP2 -> sinal do servo
#define BTN_A_PIN     5   // GP5 -> +10°
#define BTN_B_PIN     6   // GP6 -> -10°


#define SERVO_MIN_US  500.0f       // use 1000–2000us p/ curso seguro
#define SERVO_MAX_US  2500.0f


#define STEP_DEG      20.0f
#define DEBOUNCE_MS   180


static volatile float g_angle_deg = 90.0f;
static volatile uint32_t last_press_A = 0, last_press_B = 0;
static volatile bool req_inc = false, req_dec = false;
static volatile bool busy = false;   //trava durante movimento


// Protótipo da ISR
void gpio_irq_handler(uint gpio, uint32_t events);


// ===== PWM =====
static inline void servo_pwm_init(void){
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 125.0f);    // 1 tick = 1us
    pwm_config_set_wrap(&cfg, 20000 - 1);   // 20ms
    pwm_init(slice, &cfg, true);
}


static inline void servo_write_angle(float deg){
    if (deg < 0.0f)   deg = 0.0f;
    if (deg > 180.0f) deg = 180.0f;
    float us = SERVO_MIN_US + (deg/180.0f)*(SERVO_MAX_US - SERVO_MIN_US);
    uint slice = pwm_gpio_to_slice_num(SERVO_PIN);
    uint chan  = pwm_gpio_to_channel(SERVO_PIN);
    pwm_set_chan_level(slice, chan, (uint16_t)us);
    printf("Ângulo: %.1f°\n", deg);
}


// ===== Botões =====
static void buttons_init(void){
    gpio_init(BTN_A_PIN); gpio_set_dir(BTN_A_PIN, GPIO_IN); gpio_pull_up(BTN_A_PIN);
    gpio_init(BTN_B_PIN); gpio_set_dir(BTN_B_PIN, GPIO_IN); gpio_pull_up(BTN_B_PIN);


    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true);
}


// ISR leve com trava
void gpio_irq_handler(uint gpio, uint32_t events){
    if ((events & GPIO_IRQ_EDGE_FALL) == 0) return;
    uint32_t now = to_ms_since_boot(get_absolute_time());


    if (busy) return; // ignora cliques enquanto servo está em movimento


    if (gpio == BTN_A_PIN){
        if (now - last_press_A > DEBOUNCE_MS){
            req_inc = true;
            last_press_A = now;
        }
    } else if (gpio == BTN_B_PIN){
        if (now - last_press_B > DEBOUNCE_MS){
            req_dec = true;
            last_press_B = now;
        }
    }
}


int main(){
    stdio_init_all();
    servo_pwm_init();
    buttons_init();


    servo_write_angle(g_angle_deg); // inicial = 90°


    while (true){
        if (req_inc){
            req_inc = false;
            busy = true;  // 🔒 trava
            g_angle_deg += STEP_DEG;
            if (g_angle_deg > 180.0f) g_angle_deg = 180.0f;
            servo_write_angle(g_angle_deg);
            sleep_ms(200); // tempo p/ servo responder
            busy = false; // libera
        }
        if (req_dec){
            req_dec = false;
            busy = true;  // trava
            g_angle_deg -= STEP_DEG;
            if (g_angle_deg < 0.0f) g_angle_deg = 0.0f;
            servo_write_angle(g_angle_deg);
            sleep_ms(200); // tempo p/ servo responder
            busy = false; // libera
        }


        sleep_ms(1);
    }
}