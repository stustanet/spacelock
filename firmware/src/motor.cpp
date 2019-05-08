#include "motor.h"

#include "time.h"

StepperMotor::StepperMotor(
    OutputPin pin_step,
    OutputPin pin_sleep,
    OutputPin pin_direction,
    std::array<OutputPin, 3> pins_modesel,
    InputPin pin_clockwise_end,
    InputPin pin_counterclockwise_end
) :
    pin_step{pin_step},
    pin_sleep{pin_sleep},
    pin_direction{pin_direction},
    pins_modesel{pins_modesel},
    pin_clockwise_end{pin_clockwise_end},
    pin_counterclockwise_end{pin_counterclockwise_end},
    current_mode{0}
{
    this->pin_step.low();
    this->pin_sleep.low();
}

StepperMotor::~StepperMotor()
{
    this->pin_sleep.low();
}

void StepperMotor::microstep_modesel(uint8_t microstep_mode)
{
    this->pins_modesel[0].set(microstep_mode & (1 << 0));
    this->pins_modesel[1].set(microstep_mode & (1 << 1));
    this->pins_modesel[2].set(microstep_mode & (1 << 2));
}

void StepperMotor::set_mode(int8_t mode)
{
    if (mode == 0) {
        this->current_mode = 0;
        this->pin_sleep.low();
        return;
    }

    if (current_mode == 0) {
        // leave sleep mode
        this->pin_sleep.high();
        sleep_us(this->sleep_wakeup_time_us);
    }

    if (mode < 0) {
        mode *= -1;
        this->pin_direction.low();
        this->endstop_pin = &this->pin_counterclockwise_end;
    } else {
        this->pin_direction.high();
        this->endstop_pin = &this->pin_clockwise_end;
    }

    this->microsteps_per_step = mode;

    switch (mode) {
    case 1:  this->microstep_modesel(0); break;
    case 2:  this->microstep_modesel(1); break;
    case 4:  this->microstep_modesel(2); break;
    case 8:  this->microstep_modesel(3); break;
    case 16: this->microstep_modesel(4); break;
    case 32: this->microstep_modesel(5); break;
    default: this->microstep_modesel(0); this->microsteps_per_step = 1; break; // you suck
    }
}

void StepperMotor::rotate(uint32_t urevs, uint32_t urev_per_second) {
    uint32_t microsteps = (
        static_cast<uint64_t>(
            static_cast<uint64_t>(this->microsteps_per_step)
            *
            static_cast<uint64_t>(urevs)
        )
        /
        static_cast<uint64_t>(this->urev_per_step)
    );

    if (microsteps == 0) { return; }

    uint32_t microstep_period_us = (
        static_cast<uint64_t>(
            static_cast<uint64_t>(1000000)
            *
            static_cast<uint64_t>(this->urev_per_step)
        )
        /
        static_cast<uint64_t>(
            static_cast<uint64_t>(this->microsteps_per_step)
            *
            static_cast<uint64_t>(urev_per_second)
        )
    );

    if (microstep_period_us < 1) { microstep_period_us = 1; }

    Period period_timer(microstep_period_us);

    while (!this->endstop_pin->get())
    {
        this->pin_step.high();
        sleep_us(this->pin_hold_time_us);
        this->pin_step.low();
        sleep_us(this->pin_hold_time_us);

        if (--microsteps == 0) { return; }
        period_timer.wait_next();
    }
}