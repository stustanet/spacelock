#include "time.h"

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

// before the timestamp has been set, the offset will be 0,
// i.e. get_timestamp() will return (1970-01-01 00:00 UTC + time since boot)
static uint64_t timestamp_offset = 0;

static uint64_t get_timestamp_offset() {
    CriticalSectionLock lock;
    return timestamp_offset;
}

uint64_t get_timestamp() {
    return (time_get_64() / static_cast<uint64_t>(1000000)) + get_timestamp_offset();
}

void set_timestamp(uint64_t monotonic_timestamp, uint64_t unix_timestamp) {
    uint64_t microseconds_since_boot = monotonic_timestamp;
    uint64_t seconds_since_boot = microseconds_since_boot / static_cast<uint64_t>(1000000);

    timestamp_offset = unix_timestamp - seconds_since_boot;
}