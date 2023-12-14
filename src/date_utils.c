#include <math.h>
#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"
#include "date_utils.h"

void print_time_us(int64_t t64) {
    long long seconds = t64 / 1000000;
    long long us = t64 - seconds * 1000000;
    printf("seconds %lld.%06lld\n", seconds, us);
}

void print_absolute_time(absolute_time_t abstime) {
    uint64_t t64 = to_us_since_boot(abstime);
    print_time_us(t64); 
}

void print_date_time(time_t time) {
    struct tm *utc = gmtime(&time);
    int year = utc->tm_year+1900;
    int month = utc->tm_mon+1;
    int day = utc->tm_mday;
    int hour = utc->tm_hour;
    int minute = utc->tm_min;
    int second = utc->tm_sec;
    printf("%d/%d/%d %d:%d:%d\n", month, day, year, hour, minute, second);
}

void print_data_time(double fseconds_since_1970) {
    uint32_t whole_seconds = floor(fseconds_since_1970);
    double second_fraction = fseconds_since_1970 - whole_seconds;
    uint32_t ms = 1000 - second_fraction * 1000;

    time_t tt = (time_t)whole_seconds;
    struct tm *utc = gmtime(&tt);
    int year = utc->tm_year+1900;
    int month = utc->tm_mon+1;
    int day = utc->tm_mday;
    int hour = utc->tm_hour;
    int minute = utc->tm_min;
    int second = utc->tm_sec;
    printf("%d/%d/%d %d:%d:%d %dms\n", month, day, year, hour, minute, second, ms);
}

int is_leap_year(int year) {
    return (year % 4 ==  0) && (year % 100 != 0 || year % 400 ==  0);
}

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
// The doomsday calculation of the day of week is taken from
// https://github.com/bradleypallen/doomsday/tree/master
int day_of_week(int day, int month, int year) {
    int dow = (doomscentury(year) + doomsyear(year) + doomsmonth(year, month, day)) % 7;
    if (dow < 0)
        dow += 7;
    printf("dow=%d %d/%d/%d\n", dow, month, day, year);
    return dow;
}

int day_of_year(int day, int month, int year) {
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

int is_daylight_savings_time(time_t time) {
    struct tm *utc = gmtime(&time);

    int year = utc->tm_year+1900;
    int month = utc->tm_mon+1;
    int day = utc->tm_mday;
    
    // according to NIST
    // begins at 2:00 a.m. local time on the second Sunday of March
    // ends at 2:00 a.m. local time on the first Sunday of November
    //https://www.nist.gov/pml/time-and-frequency-division/popular-links/daylight-saving-time-dst

    if (month <= 2 || 12 <= month) return 0;
    if (4 <= month && month <= 10) return 1;

    // only march and november left
    int dow = day_of_week(day, month, year);
    if (month == 3) {
        return (day - dow > 7);
    } else {
        // month,  11
        return (day - dow <= 0);
    }
}
