#include "hardware.h"

#include <array>

#include "main.h"

static uint64_t timer_extended_bits = 0;

uint64_t time_get_64() {
    CriticalSectionLock lk;
    return time_get_64_isr();
}

static uint64_t prev_result = 0;


// extended bits                      timer
// 00000000000000000000000000         00000000
// 00000000000000000000000000         01111111
// 00000000000000000000000001         10000000       inc
// 00000000000000000000000001         10000000
// 00000000000000000000000001         11111111
// 00000000000000000000000010         00000000       inc
// 00000000000000000000000010         00000010


uint64_t time_get_64_isr() {
    uint16_t timer_val = time_get_16();

    bool timer_msb = timer_val & 0x8000;

    if ((timer_extended_bits & 1) != timer_msb) {
        timer_extended_bits += 1;
    }

    uint64_t result = (
        static_cast<uint64_t>(timer_extended_bits << 15) |
        static_cast<uint64_t>(timer_val)
    );

    if (result < prev_result) {
        // wtf this is supposed to be monotonic
        // this can happen if interrupts were blocked for > 30ms

        // let's try to restore monotonoty...
        result += 0x10000;
        timer_extended_bits += 2;
    }
    prev_result = result;

    return result;
}

void timer_update_extended_bits() {
    /**
    bool timer_msb = time_get_16() & 0x8000;

    if ((timer_extended_bits & 1) != timer_msb) {
        timer_extended_bits += 1;
    }
    */
    time_get_64_isr();
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

UARTTxBuffer txbuf;

void uart_transmit_next() {
    if (!(huart1.Instance->SR & USART_SR_TXE)) {
        // currently transmitting
        return;
    }

    int byte = txbuf.get_next();
    if (byte < 0) {
        // we're done. manually reset the TC bit,
        // otherwise we'll get an infinite interrupt loop.
        huart1.Instance->SR &= ~USART_SR_TC;
    } else {
        // send the next byte
        huart1.Instance->DR = byte;
    }
}

void uart_writeline(const char *text) {
    txbuf.add(text);
    txbuf.add('\r');
    txbuf.add('\n');
    uart_transmit_next();
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

int UARTTxBuffer::get_next() {
    if (this->start_pos == this->end_pos) { return -1; }
    uint8_t result = this->buf[this->start_pos];
    start_pos = (start_pos + 1) % this->buf.size();
    return result;
}

bool UARTTxBuffer::add(uint8_t byte) {
    CriticalSectionLock lk;

    uint32_t new_end_pos = (this->end_pos + 1) % this->buf.size();
    if (new_end_pos == this->start_pos) {
        // the tx buffer is full
        return false;
    }

    this->buf[this->end_pos] = byte;
    this->end_pos = new_end_pos;
    return true;
}

bool UARTTxBuffer::add(const char *buf) {
    while (*buf) {
        if (!this->add(static_cast<uint8_t>(*(buf++)))) { return false; }
    }
    return true;

}

bool UARTTxBuffer::add(const uint8_t *buf, uint32_t len) {
    while (len-- > 0) {
        if (!this->add(*(buf++))) { return false; }
    }
    return true;
}