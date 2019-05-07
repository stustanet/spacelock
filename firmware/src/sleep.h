#include <cstdint>

#ifndef __cplusplus
#error lolnope
#endif

void sleep_us(uint64_t duration_us);
void sleep_until_us(uint64_t timestamp_us);

class Period {
public:
    Period(uint64_t period_us);
    void wait_next();

private:
    uint64_t start;
    uint64_t period_us;
};