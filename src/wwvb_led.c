#include "pico/stdlib.h"
// #include "hardware/gpio.h"
#include "wwvb_led.h"
#include "boards/pico.h"
#ifndef PICO_DEFAULT_LED_PIN
#error wwvb_led example requires a board with a regular LED
#endif

#define LED_1 4
#define LED_2 5
#define LED_4 6

static void init(int led_pin) {
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, 0);
}

void wwvb_led_init() {
    init(PICO_DEFAULT_LED_PIN);
    init(LED_1);
    init(LED_2);
    init(LED_4);
}

void wwvb_led_off() {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
}

void wwvb_led_on() {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
}

static void led_progress_off() {
    gpio_put(LED_1, 0);
    gpio_put(LED_2, 0);
    gpio_put(LED_4, 0);
}

void led_progress_ok(int p) {
    gpio_put(LED_1, (p>>0)&1);
    gpio_put(LED_2, (p>>1)&1);
    gpio_put(LED_4, (p>>2)&1);
}

void led_progress_error(int p) {
    for(int i=0; i<10; ++i) {
        led_progress_ok(p);
        sleep_ms(250);
        led_progress_off();
        sleep_ms(250);
    }
}