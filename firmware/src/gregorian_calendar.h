#pragma once

#include <cstdint>

class GregorianYear {
public:
    inline GregorianYear(uint32_t year) {
        this->set(year);
    }

    void set(uint32_t year);

    inline uint32_t get() { return this->year; }

    inline uint32_t length_days() {
        if (this->is_leap_year) { return 366; } else { return 365; }
    }

    inline uint8_t month_length(uint8_t month) {
        switch (month) {
        case  1: return 31;
        case  2: return this->is_leap_year ? 29 : 28;
        case  3: return 31;
        case  4: return 30;
        case  5: return 31;
        case  6: return 30;
        case  7: return 31;
        case  8: return 31;
        case  9: return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
        default: return 0;
        }
    }

    inline uint32_t days_before_month(uint8_t month) {
        switch (month) {
        case  1: return 0;
        case  2: return 31;
        case  3: return this->is_leap_year ?  60 :  59;
        case  4: return this->is_leap_year ?  91 :  90;
        case  5: return this->is_leap_year ? 121 : 120;
        case  6: return this->is_leap_year ? 152 : 151;
        case  7: return this->is_leap_year ? 182 : 181;
        case  8: return this->is_leap_year ? 213 : 212;
        case  9: return this->is_leap_year ? 244 : 243;
        case 10: return this->is_leap_year ? 274 : 273;
        case 11: return this->is_leap_year ? 305 : 304;
        case 12: return this->is_leap_year ? 335 : 334;
        default: return 0;
        }
    }

    // note that this counts from 1, not from 0
    inline uint32_t day_of_year(uint8_t month, uint8_t day) {
        return days_before_month(month) + day;
    }

    // calculates the number of complete days between epoch and start of
    // January 1st of this year.
    uint32_t days_since_epoch();

    // calculates the number of complete days between epoch and start of
    // this day
    uint32_t days_since_epoch(uint8_t month, uint8_t day);

    // calculates the day of week (1 = monday, 7 = sunday)
    uint8_t day_of_week(uint8_t month, uint8_t day);

    // calculates the unix timestamp
    uint64_t timestamp(uint8_t month, uint8_t day);

private:
    uint32_t year;
    bool is_leap_year;

    // the timestamp of the epoch
    static constexpr const uint64_t EPOCH_TIMESTAMP = 946684800;
};