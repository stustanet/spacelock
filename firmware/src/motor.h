#pragma once
#include <cmath>
#include <array>
#include <utility>

#include "pin.h"
#include "hardware.h"
#include "config.h"

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

    static constexpr uint32_t urev_per_step = UREV_PER_STEP;
    static constexpr uint32_t sleep_wakeup_time_us = 1700;
    static constexpr uint32_t pin_hold_time_us = 2;
    static constexpr uint32_t motor_revs_to_ukey_revs = MOTOR_REVS_TO_UKEY_REVS; 

    void set_mode(int8_t mode);
    // returns a tuple of
    // (end stop state at end of execution,
    //  number of steps driven since end stop timestamp).
    // if the end stop state is NONE, the step number will always be 0.
    std::pair<EndStopState, uint32_t> rotate(uint32_t urevs, uint32_t urev_per_second, int32_t consider_endstop_after_urev);

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

    inline uint32_t urev_to_microsteps(uint32_t urev) {
        return (
            static_cast<uint64_t>(
                static_cast<uint64_t>(this->microsteps_per_step)
                *
                static_cast<uint64_t>(urev)
                *
                static_cast<uint64_t>(motor_revs_to_ukey_revs)
                /
                1'000'000
            )
            /
            static_cast<uint64_t>(this->urev_per_step)
        );
    }

    inline uint32_t microsteps_to_urev(uint32_t microsteps) {
        return (
            static_cast<uint64_t>(
                static_cast<uint64_t>(this->urev_per_step)
                *
                static_cast<uint64_t>(microsteps)
                *
                1'000'000
                /
                static_cast<uint64_t>(motor_revs_to_ukey_revs)
            )
            /
            static_cast<uint64_t>(this->microsteps_per_step)
        );
    }

    inline uint32_t urev_per_second_to_microstep_period_us(uint32_t urev_per_second) {
        return (
            static_cast<uint64_t>(
                static_cast<uint64_t>(1'000'000)
                *
                static_cast<uint64_t>(this->urev_per_step)
            )
            /
            static_cast<uint64_t>(
                static_cast<uint64_t>(this->microsteps_per_step)
                *
                static_cast<uint64_t>(urev_per_second)
                *
                static_cast<uint64_t>(motor_revs_to_ukey_revs)
                /
                1'000'000
            )
        );
    }
};
