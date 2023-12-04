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

static void test_dow();
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
static inline int day_of_week(int day, int month, int year);
static inline int is_daylight_savings_time(int day, int month, int year);

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

int main() {
    int status;
    struct tm utc;

    init_clocks();

    stdio_init_all();

    for(int i=0; i<10;++i) {
        printf("pici_wwvb start %d %s %s\n", i, WIFI_SSID, WIFI_PASSWORD);
        sleep_ms(1000);
    }
    wwvb_led_init();
    wwvb_pwm_init();

    printf("flash_block=%x\n", flash_block);
    ntp_start(progress);
    test_dow();
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

static inline void broadcast_time(
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
    int dst = is_daylight_savings_time(day, month, year);
    printf("dst=%d\n", dst);
    printf("%d %d %d %d %d %d\n", year, month, day, hour, minute, second);

    printf("mon=%d day=%d\n", month, day);
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

static inline int is_leap_year(int year) {
    return (year % 4 ==  0) && (year % 100 != 0 || year % 400 ==  0);
}

static int month_key(bool is_leap_year, int month) {
    switch(month) {
        case 1: return is_leap_year ? 0 : 1;
        case 2: return is_leap_year ? 3  : 4;
        case 3: return 4;
        case 4: return 0;
        case 5: return 2;
        case 6: return 5;
        case 7: return 0;
        case 8: return 3;
        case 9: return 6;
        case 10: return 1;
        case 11: return 4;
        case 12: return 6;
        default: return 0;
    }
}

#if 0
static inline int day_of_week(long day, long month, long year) {
#if 0
    // https://en.wikipedia.org/wiki/Julian_day#Julian_day_number_calculation
    long jdn = (1461 * (year + 4800 + (month - 14)/12))/4 +(367 * (month - 2 - 12 * ((month - 14)/12)))/12 - (3 * ((year + 4900 + (month - 14)/12)/100))/4 + day - 32075;
    double jf = (hour-12)/24.0 + minute/1440 + second/86400;
    long jd =  jdn + jf + (jf < 12 ? 1 : 0);
    printf("jh=%lf, jd %ld\n", jh, jd);
    // 0: Sunday
    // 6: Saturday
    int dow = (jd + 1) % 7;
    printf("%ld/%ld/%ld dow=%d\n", month, day, year, dow);
#else
    // https://www.almanac.com/how-find-day-week
    int yh = year/100;
    int dow = (yh + yh/4 + day + month_key(is_leap_year(year), month) - (year < 2000 ? 0 : 1)) % 7;
#endif
    printf("%ld/%ld/%ld dow=%d\n", month, day, year, dow);
    return dow;
}
#endif

#if 1
// The first term in the doomsday.day_of_week() calculation, using
// the formula given in http://en.wikipedia.org/wiki/Doomsday_rule#Finding_a_century.27s_anchor_day.
static int  doomscentury(int year) {
    int thursday = 4;
    int c = year/100 + 1;
    return ((((5 * c) + ((c - 1) / 4)) % 7) + thursday) % 7;
}

// The second term in the doomsday.day_of_week() calculation, using
// Fong and Walters' Odds+11 method from http://arxiv.org/abs/1010.0765.
static int doomsyear(int year) {
    int x = year%100;
    if (x % 2 ==  1)
        x = x + 11;
    x = x / 2;
    if (x % 2 ==  1)
        x = x + 11;
    x = x % 7;
    return (7 - x) % 7;
}

//  The third term in the doomsday.day_of_week() calculation, calculating
//  the difference in days between the day of the month in question and
//  a Doomsday during the given month.
static int doomsmonth(int year, int month, int day) {
    // In January and February, the doomsday used depends
    // on whether or not the year of the date is a leap year.
    if (month ==  1) {
        if (is_leap_year(year))
            return day - 11;
        else
           return day - 10;
    } else if (month ==  2) {
        if (is_leap_year(year))
            return day - 22;
        else
            return day - 21;
    // In March we use the 7th as our reference Doomsday.
    } else if (month ==  3)
        return day - 7;
    // Even months after March use the day of the month
    // equal to the month of the year
    else if (month % 2 ==  0)
        return day - month;
    // For the remaining months, we use the "9 to 5 at the 7-11"
    // mnemonic due to Conway
    else if (month ==  5)
        return day - 9;
    else if (month ==  9)
        return day - 5;
    else if (month ==  7)
        return day - 11;
    else // month ==  11
        return day - 7;
}

// The key equation for the Doomsday algorithm per Fong and Walters in
// http://arxiv.org/abs/1010.0765.
static inline int day_of_week(int day, int month, int year) {
    int dow = (doomscentury(year) + doomsyear(year) + doomsmonth(year, month, day)) % 7;
    if (dow < 0)
        dow += 7;
    printf("dow=%d %d/%d/%d\n", dow, month, day, year);
    return dow;
}
#endif

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

static void dow_assert(int actual, int expected) {
    if (actual != expected) {
        progress(-7);
        for(;;)
            tight_loop_contents();
    }
}

static void test_dow() {
    dow_assert(day_of_week(1, 1, 1901), 2);
    dow_assert(day_of_week(9, 9, 1650), 5);
    dow_assert(day_of_week(15, 12, 2317), 6);
    dow_assert(day_of_week(17, 10, 2202), 0);
    dow_assert(day_of_week(23, 2, 1720), 5);
    dow_assert(day_of_week(18, 3, 2197), 6);
    dow_assert(day_of_week(17, 9, 2006), 0);
    dow_assert(day_of_week(18, 3, 2175), 6);
    dow_assert(day_of_week(19, 6, 2344), 1);
    dow_assert(day_of_week(13, 3, 2458), 3);
    dow_assert(day_of_week(20, 9, 1623), 3);
    dow_assert(day_of_week(29, 6, 2018), 5);
    dow_assert(day_of_week(8, 7, 1912), 1);
    dow_assert(day_of_week(1, 5, 2096), 2);
    dow_assert(day_of_week(9, 4, 1879), 3);
    dow_assert(day_of_week(31, 7, 2258), 6);
    dow_assert(day_of_week(26, 4, 2198), 4);
    dow_assert(day_of_week(7, 7, 1828), 1);
    dow_assert(day_of_week(20, 6, 2169), 2);
    dow_assert(day_of_week(14, 7, 1648), 2);
    dow_assert(day_of_week(19, 3, 2413), 2);
    dow_assert(day_of_week(20, 2, 2056), 0);
    dow_assert(day_of_week(28, 8, 2041), 3);
    dow_assert(day_of_week(28, 5, 1842), 6);
    dow_assert(day_of_week(11, 10, 1680), 5);
    dow_assert(day_of_week(29, 9, 2571), 0);
    dow_assert(day_of_week(27, 1, 1834), 1);
    dow_assert(day_of_week(11, 1, 2399), 1);
    dow_assert(day_of_week(13, 1, 1682), 2);
    dow_assert(day_of_week(13, 9, 2314), 0);
    dow_assert(day_of_week(30, 7, 1769), 0);
    dow_assert(day_of_week(31, 10, 1924), 5);
    dow_assert(day_of_week(28, 1, 2592), 6);
    dow_assert(day_of_week(23, 12, 1900), 0);
    dow_assert(day_of_week(1, 9, 1978), 5);
    dow_assert(day_of_week(26, 10, 1713), 4);
    dow_assert(day_of_week(24, 8, 2058), 6);
    dow_assert(day_of_week(9, 2, 1849), 5);
    dow_assert(day_of_week(25, 8, 1892), 4);
    dow_assert(day_of_week(1, 11, 2130), 3);
    dow_assert(day_of_week(20, 9, 1784), 1);
    dow_assert(day_of_week(14, 8, 2315), 6);
    dow_assert(day_of_week(11, 6, 2272), 2);
    dow_assert(day_of_week(28, 1, 1680), 0);
    dow_assert(day_of_week(2, 12, 2015), 3);
    dow_assert(day_of_week(6, 12, 2278), 5);
    dow_assert(day_of_week(1, 2, 1624), 4);
    dow_assert(day_of_week(15, 11, 1979), 4);
    dow_assert(day_of_week(20, 1, 2251), 1);
    dow_assert(day_of_week(10, 1, 1617), 2);
    dow_assert(day_of_week(1, 1, 2098), 3);
    dow_assert(day_of_week(7, 11, 1770), 3);
    dow_assert(day_of_week(9, 8, 1824), 1);
    dow_assert(day_of_week(31, 1, 1592), 5);
    dow_assert(day_of_week(2, 2, 2020), 0);
    dow_assert(day_of_week(11, 1, 1817), 6);
    dow_assert(day_of_week(27, 4, 1967), 4);
    dow_assert(day_of_week(3, 9, 1673), 0);
    dow_assert(day_of_week(25, 7, 2379), 3);
    dow_assert(day_of_week(28, 6, 2169), 3);
    dow_assert(day_of_week(7, 10, 1680), 1);
    dow_assert(day_of_week(17, 12, 2370), 4);
    dow_assert(day_of_week(2, 11, 2245), 0);
    dow_assert(day_of_week(12, 7, 2553), 4);
    dow_assert(day_of_week(1, 5, 2185), 0);
    dow_assert(day_of_week(13, 12, 1714), 4);
    dow_assert(day_of_week(2, 3, 2419), 6);
    dow_assert(day_of_week(8, 12, 1705), 2);
    dow_assert(day_of_week(1, 10, 1645), 0);
    dow_assert(day_of_week(15, 4, 2488), 4);
    dow_assert(day_of_week(5, 7, 2564), 4);
    dow_assert(day_of_week(6, 4, 2229), 1);
    dow_assert(day_of_week(10, 10, 2238), 3);
    dow_assert(day_of_week(21, 11, 1648), 6);
    dow_assert(day_of_week(5, 3, 1753), 1);
    dow_assert(day_of_week(15, 11, 2193), 5);
    dow_assert(day_of_week(25, 10, 1702), 3);
    dow_assert(day_of_week(21, 3, 1786), 2);
    dow_assert(day_of_week(6, 3, 2391), 3);
    dow_assert(day_of_week(12, 6, 2391), 3);
    dow_assert(day_of_week(14, 11, 1940), 4);
    dow_assert(day_of_week(11, 1, 2452), 4);
    dow_assert(day_of_week(12, 2, 2197), 0);
    dow_assert(day_of_week(13, 11, 1909), 6);
    dow_assert(day_of_week(25, 7, 1927), 1);
    dow_assert(day_of_week(2, 1, 1792), 1);
    dow_assert(day_of_week(19, 10, 2448), 1);
    dow_assert(day_of_week(4, 1, 1796), 1);
    dow_assert(day_of_week(5, 4, 2447), 5);
    dow_assert(day_of_week(21, 5, 1980), 3);
    dow_assert(day_of_week(19, 9, 1919), 5);
    dow_assert(day_of_week(16, 8, 1868), 0);
    dow_assert(day_of_week(3, 10, 1880), 0);
    dow_assert(day_of_week(29, 8, 1690), 2);
    dow_assert(day_of_week(7, 4, 2354), 3);
    dow_assert(day_of_week(27, 6, 2356), 3);
    dow_assert(day_of_week(31, 7, 2580), 1);
    dow_assert(day_of_week(20, 5, 2481), 2);
    dow_assert(day_of_week(6, 6, 1978), 2);
    dow_assert(day_of_week(19, 9, 2480), 4);
    dow_assert(day_of_week(23, 11, 2233), 6);
    dow_assert(day_of_week(5, 4, 2350), 3);
    dow_assert(day_of_week(27, 12, 1902), 6);
    dow_assert(day_of_week(4, 6, 1631), 3);
    dow_assert(day_of_week(15, 9, 2157), 4);
    dow_assert(day_of_week(19, 8, 1806), 2);
    dow_assert(day_of_week(20, 4, 1795), 1);
    dow_assert(day_of_week(16, 10, 1747), 1);
    dow_assert(day_of_week(2, 1, 2381), 5);
    dow_assert(day_of_week(21, 1, 2191), 5);
    dow_assert(day_of_week(1, 11, 2146), 2);
    dow_assert(day_of_week(18, 11, 1644), 5);
    dow_assert(day_of_week(21, 9, 1971), 2);
    dow_assert(day_of_week(9, 1, 1724), 0);
    dow_assert(day_of_week(13, 10, 1666), 3);
    dow_assert(day_of_week(7, 6, 2185), 2);
    dow_assert(day_of_week(7, 5, 1584), 1);
    dow_assert(day_of_week(21, 11, 2435), 3);
    dow_assert(day_of_week(4, 6, 1784), 5);
    dow_assert(day_of_week(14, 3, 2270), 1);
    dow_assert(day_of_week(30, 8, 2494), 1);
    dow_assert(day_of_week(18, 9, 2309), 6);
    dow_assert(day_of_week(9, 11, 1849), 5);
    dow_assert(day_of_week(6, 10, 1590), 6);
    dow_assert(day_of_week(11, 7, 2341), 5);
    dow_assert(day_of_week(1, 6, 1855), 5);
    dow_assert(day_of_week(13, 2, 2154), 3);
    dow_assert(day_of_week(17, 1, 2463), 3);
    dow_assert(day_of_week(8, 7, 1800), 2);
    dow_assert(day_of_week(22, 11, 2424), 5);
    dow_assert(day_of_week(20, 9, 1728), 1);
}

static inline int is_daylight_savings_time(int day, int month, int year) {
    // according to NIST
    // begins at 2:00 a.m. local time on the second Sunday of March
    // ends at 2:00 a.m. local time on the first Sunday of November
    //https://www.nist.gov/pml/time-and-frequency-division/popular-links/daylight-saving-time-dst

    if (month <= 2 || 12 <= month) return 0;
    if (4 <= month && month <= 10) return 1;

    // only march and november left
    int dow = day_of_week(day, month, year);
    printf("day=%d dow=%d\n", day, dow);
    if (month == 3) {
        return (day - dow > 7);
    } else {
        // month,  11
        return (day - dow <= 0);
    }
}

static inline void gen_mark() {
    wwvb_pwm_low_power();
    wwvb_led_off();
    sleep_ms(800);
    wwvb_pwm_high_power();
    wwvb_led_on();
    sleep_ms(200);
    wwvb_pwm_low_power();
    wwvb_led_off();
}

static inline void gen_zero() {
    wwvb_pwm_low_power();
    wwvb_led_off();
    sleep_ms(200);
    wwvb_pwm_high_power();
    wwvb_led_on();
    sleep_ms(800);
    wwvb_pwm_low_power();
    wwvb_led_off();
}

static inline void gen_one() {
    wwvb_pwm_low_power();
    wwvb_led_off();
    sleep_ms(500);
    wwvb_pwm_high_power();
    wwvb_led_on();
    sleep_ms(500);
    wwvb_pwm_low_power();
    wwvb_led_off();
}
