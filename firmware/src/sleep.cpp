#include "sleep.h"

#include "hardware.h"

void sleep_us(uint64_t duration_us) {
    sleep_until_us(time_get_64() + duration_us);
}

void sleep_until_us(uint64_t timestamp_us) {
    while (time_get_64() < timestamp_us) {}
}

Period::Period(uint64_t period_us)
    :
    start{time_get_64()},
    period_us(period_us)
{}

void Period::wait_next() {
    uint64_t current_time = time_get_64();
    uint64_t position_in_period = (time_get_64() - this->start) % this->period_us;

    sleep_until_us(current_time - position_in_period + this->period_us);
}