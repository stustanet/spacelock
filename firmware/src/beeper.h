#pragma once

#include <array>
#include <cmath>

#include "pin.h"

#ifndef __cplusplus
#error lolnope
#endif

class Beeper {
public:
    Beeper(
        OutputPin pin_speaker
    );

    void beep(uint32_t period_us, uint32_t duration_us);
    void pattern(uint32_t period_a_us, uint32_t period_b_us, uint32_t pause_us, uint32_t beeps);

    void good(uint32_t length_us);
    void error(uint32_t code);
    void party(uint32_t duration);

private:
    OutputPin pin_speaker;
};
