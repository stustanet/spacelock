#include "motor.h"

#include "time.h"
#include "stm32f1xx_it.h"

StepperMotor::StepperMotor(
    OutputPin pin_step,
    OutputPin pin_sleep,
    OutputPin pin_direction,
    std::array<OutputPin, 3> pins_modesel,
    OutputPin pin_reset,
    InputPin pin_fault
) :
    pin_step{pin_step},
    pin_sleep{pin_sleep},
    pin_direction{pin_direction},
    pins_modesel{pins_modesel},
    pin_reset{pin_reset},
    pin_fault{pin_fault},
    current_mode{0}
{
    this->pin_step.low();
    this->pin_sleep.low();
    this->pin_reset.high();
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
    } else {
        this->pin_direction.high();
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

std::pair<EndStopState, uint32_t> StepperMotor::rotate(uint32_t urevs, uint32_t urev_per_second, int32_t consider_endstop_after_urev) {
    endstop_detector.clear_end_stop_state();
    int32_t consider_endstop_after_microsteps = -1;
    if (consider_endstop_after_urev >= 0) {
        consider_endstop_after_microsteps = urev_to_microsteps(consider_endstop_after_urev);
        uint64_t val = consider_endstop_after_microsteps;
        uart_writeline("consider endstop after microsteps, ", &val);
    }
    uint32_t microsteps = urev_to_microsteps(urevs);
    uint32_t microstep_counter = 0;

    if (microsteps == 0) { return { EndStopState::NONE, 0}; }

    uint32_t microstep_period_us = urev_per_second_to_microstep_period_us(urev_per_second);

    if (microstep_period_us < 1) { microstep_period_us = 1; }

    Period period_timer(microstep_period_us);
    while (1)
    {
        this->pin_step.high();
        sleep_us(this->pin_hold_time_us);
        this->pin_step.low();
        sleep_us(this->pin_hold_time_us);

        if (++microstep_counter >= microsteps) { return { EndStopState::NONE, 0 }; }

        uint64_t end_stop_timestamp;
        EndStopState end_stop_state = endstop_detector.get_end_stop_state(&end_stop_timestamp);
        if (consider_endstop_after_microsteps >= 0 && microstep_counter >= consider_endstop_after_microsteps)
        {
            if (end_stop_state != EndStopState::NONE) {
                uint64_t val = microstep_counter;
                uart_writeline("endstop, ", &val);
                val = consider_endstop_after_microsteps;
                uart_writeline("endstop, ", &val);
                return { end_stop_state, microsteps_to_urev((time_get_64() - end_stop_timestamp) / microstep_period_us) };
            }
        }

        period_timer.wait_next();
    }
}
