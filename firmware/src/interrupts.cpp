#include <array>

#include "interrupts.h"
#include "hardware.h"

uint32_t systick_callback_count = 0;
std::array<callback_type, 8> systick_callbacks;

void on_systick() {
    for (uint32_t i = 0; i < systick_callback_count; i++) {
        systick_callbacks[i]();
    }
}

void add_systick_callback(callback_type cb) {
    CriticalSectionLock lock;

    if (systick_callback_count >= systick_callbacks.size()) {
        return;
    }

    systick_callbacks[systick_callback_count++] = cb;
}