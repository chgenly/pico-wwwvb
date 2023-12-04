#include <stdio.h>
#include "date_utils.h"

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



int is_daylight_savings_time(int day, int month, int year) {
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
