#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include "wwvb_led.h"

static void dow_assert(int actual, int expected) {
    if (actual != expected) {
        led_progress_error(-7);
        for(;;)
            tight_loop_contents();
    }
}

int main() {
   if (cyw43_arch_init()) {
        printf("failed to initialise wifi\n");
        led_progress_error(1);
        return 0;
    } 
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

    return 0;
}

