#pragma once
#include <cmath>
#include <array>

#include "pin.h"

#ifndef __cplusplus
#error lolnope
#endif

class StepperMotor {
public:
    StepperMotor(
        OutputPin pin_step,
        OutputPin pin_sleep,
        OutputPin pin_direction,
        std::array<OutputPin, 3> pins_modesel,
        OutputPin pin_reset,
        InputPin pin_fault
    );

    ~StepperMotor();

    static constexpr uint32_t urev_per_step = (uint32_t)(1000000 / 200);
    static constexpr uint32_t sleep_wakeup_time_us = 1700;
    static constexpr uint32_t pin_hold_time_us = 2;

    void set_mode(int8_t mode);
    void rotate(uint32_t urevs, uint32_t urev_per_second);

private:
    OutputPin pin_step;
    OutputPin pin_sleep;
    OutputPin pin_direction;
    std::array<OutputPin, 3> pins_modesel;
    OutputPin pin_reset;
    InputPin pin_fault;

    void microstep_modesel(uint8_t microstep_mode);

    int8_t current_mode;

    uint8_t microsteps_per_step;
};