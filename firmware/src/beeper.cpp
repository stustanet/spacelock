#include "beeper.h"

#include "time.h"

Beeper::Beeper(
    OutputPin pin_speaker
) :
    pin_speaker{pin_speaker}
{
    this->pattern(1000000/200, 1000000/150, 100000, 5);
}


void Beeper::beep(uint32_t period_us, uint32_t duration_us)
{
    uint32_t num_periods = duration_us / period_us;
    
    while (num_periods--) {
        this->pin_speaker.high();
        sleep_us(period_us/2);
        this->pin_speaker.low();
        sleep_us(period_us/2);
    }
}


void Beeper::pattern(uint32_t period_a_us, uint32_t period_b_us, uint32_t pause_us, uint32_t beeps)
{
    while (beeps--) {
        this->beep(period_a_us, 10000);

        uint32_t tmp = period_a_us;
        period_a_us = period_b_us;
        period_b_us = tmp;

        if (beeps) {
            sleep_us(pause_us);
        }
    }
}


void Beeper::good(uint32_t length_us)
{
    this->beep(1000000/152, length_us);
}


void Beeper::error(uint32_t code)
{
    while (code--) {
        this->pattern(1000000/152, 1000000/900, 1000, 10);
        sleep_us(200000);
    }
}

void Beeper::party(uint32_t duration)
{
    this->pattern(1000000/152, 1000000/900, 100000, duration);
}
