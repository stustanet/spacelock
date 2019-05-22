#include "dcf77.h"

#include <bitset>

#include "dcf77_analyze.h"
#include "gregorian_calendar.h"
#include "hardware.h"
#include "interrupts.h"
#include "time.h"

static void dcf77_update();

// the bits that were received this minute
// a normal minute has 59 bits, but minutes where a leap second is inserted
// have 60 bits.
static std::bitset<60> rx_bits;
static uint8_t rx_bitcount = 0;
static InputPin *input_pin;
static bool last_input = false;
static uint32_t last_input_time;

void dcf77_init(InputPin *pin) {
    input_pin = pin;
    last_input_time = time_get_64();
    add_systick_callback(dcf77_update);
}

static void dcf77_update() {
    bool input = input_pin->get();
    if (input == last_input) { return; }
    last_input = input;

    uint64_t edge_timestamp = time_get_64_isr();
    uint32_t time_delta = static_cast<uint32_t>(edge_timestamp) - last_input_time;
    last_input_time = static_cast<uint32_t>(edge_timestamp);

    if (input) {
        // Rising edge; analyze length of the negative duty cycle:
        //
        //  0.7s - 1.0s: regular second
        //  1.7s - 2.0s: start of new minute

        if ((time_delta > 700000) && (time_delta <= 1000000)) {
            // It was just a regular second; everything is alright.
        } else if ((time_delta >= 1700000) && (time_delta <= 2000000)) {
            // Alright; this minute is done.
            // Time to pass the accumulated bits on for analyzing.
            // (this includes checking whether there are any/the
            //  right number of bits and whether they make the
            //  tiniest bit of sense).
            if (dcf77_analyze(rx_bits, rx_bitcount, edge_timestamp)) {
                set_timestamp(edge_timestamp);
            }
            rx_bitcount = 0;
        } else {
            // This is an illegal duty cycle; the signal has
            // been corrupted.

            // Better luck next minute.
            rx_bitcount = 0;
        }
    } else {
        // Falling edge (bit has been received).
        if (rx_bitcount > 60) {
            // Something is awfully wrong here. Maybe we missed
            // the minute-end marker.

            // discard the received bits.
            rx_bitcount = 0;
        }

        // Analyze length of the positive duty cycle:
        //
        //  0.07s - 0.13s: bit 0
        //  0.17s - 0.23s: bit 1

        if ((time_delta >= 70000) && (time_delta <= 150000)) {
            // We have received a "0" bit.
            rx_bits[rx_bitcount++] = 0;
        } else if ((time_delta >= 170000) && (time_delta <= 260000)) {
            // We have received a "1" bit.
            rx_bits[rx_bitcount++] = 1;
        } else {
            // Illegal positive duty cycle length; the signal is
            // corrupted.
            //
            // Better luck next minute.
            rx_bitcount = 0;
        }
    }
}
