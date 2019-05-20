#include "dcf77_analyze.h"

#include "gregorian_calendar.h"

// DCF77 contains two-digit years.
// all values >= 19 are interpreted as 20xx,
// all values < 19 are interpreted as 21xx.
static constexpr const uint32_t BASE_YEAR = 2005;

static constexpr const uint32_t BASE_CENTURY = (BASE_YEAR / 100) * 100;
static constexpr const uint32_t BASE_YEAR_IN_CENTURY = BASE_YEAR % 100;


uint8_t parity(const std::bitset<60> &bits, uint8_t start, uint8_t end) {
    uint8_t result = 0;
    while (start <= end) {
        if (bits[start++]) { result = !result; }
    }
    return result;
}

bool dcf77_analyze(const std::bitset<60> &bits, uint8_t bitcount, uint64_t &minute_timestamp) {
    // complete minute received
    if (bitcount < 59) { return false; }
    // on leap second: last bit always 0
    if (bitcount == 60 && bits[59] != 0) { return false; }

    // minute marker, always 0
    if (bits[0] != 0) { return false; }
    // time marker, always 1
    if (bits[20] != 1) { return false; }
    // even parity on minutes
    if (parity(bits, 21, 28) != 0) { return false; }
    // even parity on hours
    if (parity(bits, 29, 35) != 0) { return false; }
    // even parity on date
    if (parity(bits, 36, 58) != 0) { return false; }

    uint8_t utc_offset = bits[17] * 2 + bits[18];

    uint8_t minute = 0;
    if (bits[21]) { minute += 1; }
    if (bits[22]) { minute += 2; }
    if (bits[23]) { minute += 4; }
    if (bits[24]) { minute += 8; }
    if (minute >= 10) { return false; }
    if (bits[25]) { minute += 10; }
    if (bits[26]) { minute += 20; }
    if (bits[27]) { minute += 40; }
    if (minute >= 60) { return false; }

    uint8_t hour = 0;
    if (bits[29]) { hour += 1; }
    if (bits[30]) { hour += 2; }
    if (bits[31]) { hour += 4; }
    if (bits[32]) { hour += 8; }
    if (hour >= 10) { return false; }
    if (bits[33]) { hour += 10; }
    if (bits[34]) { hour += 20; }
    if (hour >= 24) { return false; }

    uint8_t day_of_month = 0;
    if (bits[36]) { day_of_month += 1; }
    if (bits[37]) { day_of_month += 2; }
    if (bits[38]) { day_of_month += 4; }
    if (bits[39]) { day_of_month += 8; }
    if (day_of_month >= 10) { return false; }
    if (bits[40]) { day_of_month += 10; }
    if (bits[41]) { day_of_month += 20; }
    if (day_of_month == 0) { return false; }

    uint8_t day_of_week = 0;
    if (bits[42]) { day_of_week += 1; }
    if (bits[43]) { day_of_week += 2; }
    if (bits[44]) { day_of_week += 4; }
    if (day_of_week > 7) { return false; }
    if (day_of_week == 0) { return false; }

    uint8_t month = 0;
    if (bits[45]) { month += 1; }
    if (bits[46]) { month += 2; }
    if (bits[47]) { month += 4; }
    if (bits[48]) { month += 8; }
    if (month >= 10) { return false; }
    if (bits[49]) { month += 10; }
    if (month > 12) { return false; }
    if (month == 0) { return false; }

    uint8_t year_of_century = 0;
    if (bits[50]) { year_of_century += 1; }
    if (bits[51]) { year_of_century += 2; }
    if (bits[52]) { year_of_century += 4; }
    if (bits[53]) { year_of_century += 8; }
    if (year_of_century >= 10) { return false; }
    if (bits[54]) { year_of_century += 10; }
    if (bits[55]) { year_of_century += 20; }
    if (bits[56]) { year_of_century += 40; }
    if (bits[57]) { year_of_century += 80; }
    if (year_of_century >= 100) { return false; }

    uint32_t year = BASE_CENTURY + year_of_century;
    if (year_of_century < BASE_YEAR_IN_CENTURY) { year += 100; }

    GregorianYear gregorian_year(year);
    for (uint8_t centuries_incremented = 0;;) {
        if (gregorian_year.day_of_week(month, day_of_month) == day_of_week) { break; }
        if (++centuries_incremented == 4) {
            // day of week doesn't match for any century
            return false;
        }
        gregorian_year.set(gregorian_year.get() + 100);
    }

    minute_timestamp = gregorian_year.timestamp(month, day_of_month);
    minute_timestamp += 3600 * hour + 60 * minute;
    minute_timestamp -= 3600 * utc_offset;
    return true;
}
