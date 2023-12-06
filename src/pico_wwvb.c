#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pico/types.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "pico/cyw43_arch.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/pwm.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "picow_ntp_client.h"
#include "wwvb_pwm.h"
#include "wwvb_led.h"
#include "hardware/flash.h"
#include "date_utils.h"

void gen_mark();
void gen_zero();
void gen_one();
void broadcast_time(
    int hour,
    int minute,
    int second,
    int day,
    int month,
    int year,
    int max_transmissions
);

int flash_block[FLASH_SECTOR_SIZE] __in_flash("group1")  __attribute__((aligned(FLASH_SECTOR_SIZE)))= {123};

void progress(int p) {
    if (p > 0)        
        led_progress_ok(p);
    else
        led_progress_error(-p);
}

static void measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_adc  = %dkHz\n", f_clk_adc);
    printf("clk_rtc  = %dkHz\n", f_clk_rtc);

    // Can't measure clk_ref / xosc as it is the ref
}

// System clock to 48 mhz.
// Peripheral clock to 48mhz.
static void init_clocks() {
    measure_freqs();

    // clocks_init();

    // // Change clk_sys to be 48MHz. The simplest way is to take this from PLL_USB
    // // which has a source frequency of 48MHz
    // clock_configure(clk_sys,
    //                 CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
    //                 CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
    //                 48 * MHZ,
    //                 48 * MHZ);

    // // Turn off PLL sys for good measure
    // pll_deinit(pll_sys);

    // // CLK peri is clocked from clk_sys so need to change clk_peri's freq
    // clock_configure(clk_peri,
    //                 0,
    //                 CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
    //                 48 * MHZ,
    //                 48 * MHZ);

    // // Re init uart now that clk_peri has changed
    // stdio_init_all();

    // measure_freqs();
}

/** This delay gives the user time to connect a terminal to look at the usb output. */
static void startup_delay() {
    for(int i=0; i<8;++i) {
        printf("pici_wwvb start %d\n", i);
        for(int j=0; j<10;++j) {
            led_progress_ok(i);
            sleep_ms(0);
            led_progress_off();
            sleep_ms(20);
        }
    }
}

int main() {
    int status;
    struct tm utc;

    stdio_init_all();
    wwvb_led_init();

    startup_delay();
    wwvb_pwm_init();
    init_clocks();

    printf("flash_block=%x\n", flash_block);
    ntp_start(progress);
    
    for(;;) {
        for(;;) {
            status = ntp_ask_for_time(&utc);
            printf("status1=%d\n", status);
            if (status)
                break;
            sleep_ms(30*1000);
        }
        printf("ntp returned\n");
        led_progress_off();

        broadcast_time(utc.tm_year+1900, utc.tm_mon+1, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec, 10);
    }    
    ntp_end();
}

void broadcast_time(
    int year,
    int month,
    int day,
    int hour,
    int minute,
    int second,
    int max_transmissions
) {
    int broadcasts = 0;
    int doy = day_of_year(day, month, year);
    int leap = is_leap_year(year);
    // Daylight saving time (DST) and standard time (ST) information is transmitted at seconds
    // 57 and 58. When ST is in effect, bits 57 and 58 are set to 0. When DST is in effect, bits
    // 57 and 58 are set to 1. On the day of a change from ST to DST bit 57 changes from 0 to
    // 1 at 0000 UTC, and bit 58 changes from 0 to 1 exactly 24 hours later. On the day of a
    // change from DST back to ST bit 57 changes from 1 to 0 at 0000 UTC, and bit 58 changes
    // from 1 to 0 exactly 24 hours later.
    int dst1 = is_daylight_savings_time(day, month, year);
    int dst2 = is_daylight_savings_time(day-1, month, year);
    printf("dst1=%d dst2=%d\n", dst1, dst2);

    while (1) {
        printf("%d %d %d %d %d %d\n", year, month, day, hour, minute, second);
        // compute bit
        unsigned char bit=0; // 2 = mark, 1 = "1", 0 = "0"
        switch (second) {
            case 0: // mark
                bit = 2;
                break;
            case 1: // minute 40
                bit = ((minute / 10) >> 2) & 1;
                break;
            case 2: // minute 20
                bit = ((minute / 10) >> 1) & 1;
                break;
            case 3: // minute 10
                bit = ((minute / 10) >> 0) & 1;
                break;
            case 4: // blank
                bit = 0;
                break;
            case 5: // minute 8
                bit = ((minute % 10) >> 3) & 1;
                break;
            case 6: // minute 4
                bit = ((minute % 10) >> 2) & 1;
                break;
            case 7: // minute 2
                bit = ((minute % 10) >> 1) & 1;
                break;
            case 8: // minute 1
                bit = ((minute % 10) >> 0) & 1;
                break;
            case 9: // mark
                bit = 2;
                break;
            case 10: // blank
                bit = 0;
                break;
            case 11: // blank
                bit = 0;
                break;
            case 12: // hour 20
                bit = ((hour / 10) >> 1) & 1;
                break;
            case 13: // hour 10
                bit = ((hour / 10) >> 0) & 1;
                break;
            case 14: // blank
                bit = 0;
                break;
            case 15: // hour 8
                bit = ((hour % 10) >> 3) & 1;
                break;
            case 16: // hour 4
                bit = ((hour % 10) >> 2) & 1;
                break;
            case 17: // hour 2
                bit = ((hour % 10) >> 1) & 1;
                break;
            case 18: // hour 1
                bit = ((hour % 10) >> 0) & 1;
                break;
            case 19: // mark
                bit = 2;
                break;
            case 20: // blank
                bit = 0;
                break;
            case 21: // blank
                bit = 0;
                break;
            case 22: // doy of year 200
                bit = ((doy / 100) >> 1) & 1;
                break;
            case 23: // doy of year 100
                bit = ((doy / 100) >> 0) & 1;
                break;
            case 24: // blank
                bit = 0;
                break;
            case 25: // doy of year 80
                bit = (((doy / 10) % 10) >> 3) & 1;
                break;
            case 26: // doy of year 40
                bit = (((doy / 10) % 10) >> 2) & 1;
                break;
            case 27: // doy of year 20
                bit = (((doy / 10) % 10) >> 1) & 1;
                break;
            case 28: // doy of year 10
                bit = (((doy / 10) % 10) >> 0) & 1;
                break;
            case 29: // mark
                bit = 2;
                break;
            case 30: // doy of year 8
                bit = ((doy % 10) >> 3) & 1;
                break;
            case 31: // doy of year 4
                bit = ((doy % 10) >> 2) & 1;
                break;
            case 32: // doy of year 2
                bit = ((doy % 10) >> 1) & 1;
                break;
            case 33: // doy of year 1
                bit = ((doy % 10) >> 0) & 1;
                break;
            case 34: // blank
                bit = 0;
                break;
            case 35: // blank
                bit = 0;
                break;
            case 36: // UTI sign +
                bit = 1;
                break;
            case 37: // UTI sign -
                bit = 0;
                break;
            case 38: // UTI sign +
                bit = 1;
                break;
            case 39: // mark
                bit = 2;
                break;
            case 40: // UTI correction 0.8
                bit = 0;
                break;
            case 41: // UTI correction 0.4
                bit = 0;
                break;
            case 42: // UTI correction 0.2
                bit = 0;
                break;
            case 43: // UTI correction 0.1
                bit = 0;
                break;
            case 44: // blank
                bit = 0;
                break;
            case 45: // year 80
                bit = (((year / 10) % 10) >> 3) & 1;
                break;
            case 46: // year 40
                bit = (((year / 10) % 10) >> 2) & 1;
                break;
            case 47: // year 20
                bit = (((year / 10) % 10) >> 1) & 1;
                break;
            case 48: // year 10
                bit = (((year / 10) % 10) >> 0) & 1;
                break;
            case 49: // mark
                bit = 2;
                break;
            case 50: // year 8
                bit = ((year % 10) >> 3) & 1;
                break;
            case 51: // year 4
                bit = ((year % 10) >> 2) & 1;
                break;
            case 52: // year 2
                bit = ((year % 10) >> 1) & 1;
                break;
            case 53: // year 1
                bit = ((year % 10) >> 0) & 1;
                break;
            case 54: // blank
                bit = 0;
                break;
            case 55: // leap year
                bit = leap ? 1 : 0;
                break;
            case 56: // leap second
                bit = 0;
                break;
            case 57: // dst bit 1
                bit = dst1 ? 1 : 0; 
                break;
            case 58: // dst bit 2
                bit = dst2 ? 1 : 0;
                break;
            case 59: // mark
                bit = 2;
                break;
        }
        // transmit bit
        if (bit == 0) {
            gen_zero();
        } else if (bit == 1) {
            gen_one();
        } else {
            gen_mark();
        }

        // increment time (or give up and ask GPS for a new time);
        if (++second >= 60) {
            second = 0;
            // quit after max_transmissions
            if (++broadcasts >= max_transmissions) {
                return;
            }
            if (++minute >= 60) {
                minute = 0;
                if (++hour >= 24) {
                    // be lazy, just ask the GPS for a new time
                    return;
                }
            }
        }
    }
}



void gen_mark() {
    wwvb_pwm_low_power();
    wwvb_led_off();
    sleep_ms(800);
    wwvb_pwm_high_power();
    wwvb_led_on();
    sleep_ms(200);
    wwvb_pwm_low_power();
    wwvb_led_off();
}

void gen_zero() {
    wwvb_pwm_low_power();
    wwvb_led_off();
    sleep_ms(200);
    wwvb_pwm_high_power();
    wwvb_led_on();
    sleep_ms(800);
    wwvb_pwm_low_power();
    wwvb_led_off();
}

void gen_one() {
    wwvb_pwm_low_power();
    wwvb_led_off();
    sleep_ms(500);
    wwvb_pwm_high_power();
    wwvb_led_on();
    sleep_ms(500);
    wwvb_pwm_low_power();
    wwvb_led_off();
}
