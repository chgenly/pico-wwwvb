
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pico/types.h>
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "pico/cyw43_arch.h"
#include "hardware/pwm.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "picow_ntp_client.h"
#include "wwvb_pwm.h"
#include "wwvb_led.h"

static inline void gen_mark();
static inline void gen_zero();
static inline void gen_one();
static inline void broadcast_time(
    int hour,
    int minute,
    int second,
    int day,
    int month,
    int year,
    int max_transmissions
);
static inline int is_leap_year(int year);
static inline int day_of_year(int day, int month, int year);
static inline int day_of_week(long day, long month, long year); // 0 => sunday
static inline int is_daylight_savings_time(int day, int month, int year);

void progress(int p) {
    if (p > 0)        
        led_progress_ok(p);
    else
        led_progress_error(-p);
}

int main() {
    int status;
    struct tm utc;

    stdio_init_all();
    wwvb_pwm_init();

    for(;;) {
        for(;;) {
            status = ntp_ask_for_time(&utc, progress);
            if (status == 0)
                break;
            sleep_ms(30*1000);
        }

        broadcast_time(utc.tm_hour, utc.tm_min, utc.tm_sec, utc.tm_mon, utc.tm_mday, utc.tm_year+1900, 10);
    }    
}

static inline void broadcast_time(
    int hour,
    int minute,
    int second,
    int day,
    int month,
    int year,
    int max_transmissions
) {
    int broadcasts = 0;
    int doy = day_of_year(day, month, year);
    int leap = is_leap_year(year);
    int dst = is_daylight_savings_time(day, month, year);
    while (1) {
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
                bit = dst ? 1 : 0; // XXX this isn't exactly correct
                break;
            case 58: // dst bit 2
                bit = dst ? 1 : 0;
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

        // increment time (or give up and ask GPS for a new time)
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

static inline int is_leap_year(int year) {
    return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

static inline int day_of_week(long day, long month, long year) {
    // via https://.wikipedia.org/wiki/Julian_day
    long a = (14 - month) / 12;
    long y = year + 4800 - a;
    long m = month + 12 * a - 3;
    long jdn = day + (153 * m + 2) / 5 + 365 * y +
        (y / 4) - (y / 100) + (y / 400) - 32045;

    return (jdn + 1) % 7;
}

static inline int day_of_year(int day, int month, int year) {
    static const short cum_days_in_month[] = {
        0,
        31,
        59,
        90,
        120,
        151,
        181,
        212,
        243,
        273,
        304,
        334
    }; // cumulative
    return cum_days_in_month[month - 1] + day + (is_leap_year(year) & (month > 2));
}

static inline int is_daylight_savings_time(int day, int month, int year) {
    // according to NIST
    // begins at 2:00 a.m. on the second Sunday of March
    // ends at 2:00 a.m. on the first Sunday of November

    if (month <= 2 || 12 <= month) return 0;
    if (4 <= month && month <= 10) return 1;

    // only march and november left
    int dow = day_of_week(day, month, year);
    if (month == 3) {
        return (day - dow > 7);
    } else {
        // month == 11
        return (day - dow <= 0);
    }
}

static inline void gen_mark() {
    wwvb_pwm_low_power();
    led_wwvb_off();
    sleep_ms(800);
    wwvb_pwm_high_power();
    led_wwvb_on();
    sleep_ms(200);
    wwvb_pwm_low_power();
    led_wwvb_off();
}

static inline void gen_zero() {
    wwvb_pwm_low_power();
    led_wwvb_off();
    sleep_ms(200);
    wwvb_pwm_high_power();
    led_wwvb_on();
    sleep_ms(800);
    wwvb_pwm_low_power();
    led_wwvb_off();
}

static inline void gen_one() {
    wwvb_pwm_low_power();
    led_wwvb_off();
    sleep_ms(500);
    wwvb_pwm_high_power();
    led_wwvb_on();
    sleep_ms(500);
    wwvb_pwm_low_power();
    led_wwvb_off();
}
