#include "hardware.h"

#include <array>

static uint64_t timer_extended_bits;

uint64_t time_get_64() {
    CriticalSectionLock lk;

    uint16_t timer_val = time_get_16();

    bool extended_lsb = timer_extended_bits & 1;
    bool timer_msb = timer_val & 0x8000;

    if (extended_lsb != timer_msb) {
        timer_extended_bits += 1;
    }

    return (timer_extended_bits << 15) | timer_val;
}

void timer_update_extended_bits() {
    bool extended_lsb = timer_extended_bits & 1;
    bool timer_msb = time_get_16() & 0x8000;

    if (extended_lsb != timer_msb) {
        timer_extended_bits += 1;
    }
}

// the UART rx buffers are double-buffered.
UARTRxBuffer rxbuf_a;
UARTRxBuffer rxbuf_b;

// this is the buffer to which newly-received bytes are stored
UARTRxBuffer *current_rxbuf = &rxbuf_a;
// this is the buffer which is being processed
UARTRxBuffer *current_procbuf = &rxbuf_b;

void uart_data_received(uint8_t byte) {
    if (byte == '\0' || byte == '\r' || byte == '\n') {
        current_rxbuf->finish();
    } else {
        current_rxbuf->add(byte);
    }
}

UARTRxBuffer *uart_poll_message() {
    CriticalSectionLock lk;

    if (!current_rxbuf->finished) { return nullptr; }

    UARTRxBuffer *tmp = current_rxbuf;

    current_rxbuf = current_procbuf;
    current_rxbuf->reset();

    current_procbuf = tmp;

    return tmp;
}