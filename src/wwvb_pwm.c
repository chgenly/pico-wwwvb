#include <pico/types.h>
#include "wwvb_pwm.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define GPIO_PIN 1
// 48 MHZ peripheral clock / 60 Khz = 800
#define PWM_TOP 2083
#define PWM_LOW_POWER 0
#define PWM_HIGH_POWER (PWM_TOP/2)

int slice_num;

void wwvb_pwm_init() {
    gpio_set_function(GPIO_PIN, GPIO_FUNC_PWM);
    
    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    slice_num = pwm_gpio_to_slice_num(GPIO_PIN);
    pwm_set_wrap(slice_num, PWM_TOP);
    pwm_set_chan_level(slice_num, PWM_CHAN_B, PWM_LOW_POWER);
    pwm_set_enabled(slice_num, true);
}

void wwvb_pwm_low_power() {
    pwm_set_chan_level(slice_num, PWM_CHAN_B, PWM_LOW_POWER);
}

void wwvb_pwm_high_power() {
    pwm_set_chan_level(slice_num, PWM_CHAN_B, PWM_HIGH_POWER);
}
