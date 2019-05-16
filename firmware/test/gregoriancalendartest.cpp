#include <cstdint>
#include <cstdio>

#include "gregorian_calendar.h"

int main() {
    uint32_t year;
    uint32_t month;
    uint32_t day;

    while (1) {
        if (scanf("%04d-%02d-%02d", &year, &month, &day) < 3) { return 0; };

        if (month > 12 || month < 1) { printf("ERR\n"); continue; }
        if (day > 31 || day < 1) { printf("ERR\n"); continue; }

        GregorianYear calendar_year(year);

        printf(
            "%03d %01d %10ld\n",
            calendar_year.day_of_year(month, day),
            calendar_year.day_of_week(month, day),
            calendar_year.timestamp(month, day)
        );
    }
}
