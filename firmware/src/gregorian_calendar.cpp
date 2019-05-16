#include "gregorian_calendar.h"

void GregorianYear::set(uint32_t year) {
    this->year = year;

    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0) {
                this->is_leap_year = true;
            } else {
                this->is_leap_year = false;
            }
        } else {
            this->is_leap_year = true;
        }
    } else {
        this->is_leap_year = false;
    }
}

uint32_t GregorianYear::days_since_epoch() {
    // in 400 years there are 146097 days.
    uint32_t years_since_epoch = this->year - 2000;

    uint32_t years_in_400_year_period = years_since_epoch % 400;
    uint32_t number_of_400_year_periods = years_since_epoch / 400;

    // from the number of completed 400-year periods
    uint32_t result = 146097 * number_of_400_year_periods;

    if (years_in_400_year_period > 0) {
        // the number of completed years in the current 400-year period
        result += 365 * years_in_400_year_period;
        // the number of completed leap years in the current 400-year period
        result += (years_in_400_year_period + 3) / 4;
        result -= (years_in_400_year_period + 99) / 100;
        result += 1;
    }

    return result;
}

uint32_t GregorianYear::days_since_epoch(uint8_t month, uint8_t day) {
    uint32_t days_to_start_of_year = this->days_since_epoch();
    uint32_t days_in_this_year = this->day_of_year(month, day) - 1;
    uint32_t result = days_to_start_of_year + days_in_this_year;
    return result;
}

uint8_t GregorianYear::day_of_week(uint8_t month, uint8_t day) {
    return (this->days_since_epoch(month, day) + 5) % 7 + 1;
}

uint64_t GregorianYear::timestamp(uint8_t month, uint8_t day) {
    return static_cast<uint64_t>(this->days_since_epoch(month, day)) * 3600 * 24 + this->EPOCH_TIMESTAMP;
}
