#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "wwvb_led.h"
#include "boards/pico.h"

#define LED_1 10
#define LED_2 11
#define LED_4 12

static void init(int led_pin) {
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, 0);
}

void wwvb_led_init() {
    init(LED_1);
    init(LED_2);
    init(LED_4);
}

void wwvb_led_off() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

void wwvb_led_on() {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

void led_progress_off() {
    gpio_put(LED_1, 0);
    gpio_put(LED_2, 0);
    gpio_put(LED_4, 0);
}

void led_progress_ok(int p) {
    gpio_put(LED_1, (p>>0)&1);
    gpio_put(LED_2, (p>>1)&1);
    gpio_put(LED_4, (p>>2)&1);
    sleep_ms(100);
}

void led_progress_error(int p) {
    bool slow;

    if (p > 7) {
        p = p & 7;
        slow = true;
    } else
        slow = false;

    for(int i=0; i<10; ++i) {
        led_progress_ok(p);
        sleep_ms(slow ? 1000 : 250);
        led_progress_off();
        sleep_ms(slow ? 2000 : 500);
    }
}