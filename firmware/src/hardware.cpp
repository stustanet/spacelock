#include "hardware.h"

#include <cstdint>
#include <array>

#include "config.h"
#include "main.h"
#include "time.h"

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

void uart_write_u16(uint16_t data){
    uint8_t a,b;
    uint8_t mask = 0b11000000;
    a = mask | (data & ~mask);
    b = mask | (data >> 6 & ~mask);
    txbuf.add(a);
    txbuf.add(b);
    uart_transmit_next();
}

void uart_writeline(const char *text, const uint64_t *param) {
    txbuf.add(text);
    if (param != nullptr) {
        bool started = false;
        for (int8_t shift = 60; shift >= 0; shift -= 4)
        {
            uint8_t digit = (*param >> shift) & 0xf;
            if (digit != 0) { started = true; }
            if (started || (shift == 0)) {
                txbuf.add("0123456789abcdef"[digit]);
            }
        }
    }
    txbuf.add('\r');
    txbuf.add('\n');
    uart_transmit_next();
}

EndStopDetector endstop_detector;

void adc_value_received(uint16_t adc_value) {
    // This is called at 125 kHz / 7 = 17857 Hz
    // ADC range 12 bits, reference 3.3V, 1 V/A -> 3.3 / (2**12 - 1) * 1.0 * 1e6
    uint32_t current_nA = adc_value * 806;
    endstop_detector.current_received(current_nA);

    // static uint8_t value_counter = 0;
    // static uint16_t value_sum = 0;
    // value_sum += adc_value;
    // value_counter += 1;
    // // output the ADC values in a compact binary format, for debug purposes
    // if (value_counter == 10) {
    //     uart_write_u16((uint16_t)(value_sum / 10));

    //     value_counter = 0;
    //     value_sum = 0;
    // }
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

void EndStopDetector::current_received(uint32_t current_uA)
{
    // currents are coming in at 125 / 7 kHz.
    // we want to filter them with a time constant of 10 ms
    // -> alpha = 1/(1-exp(-delta_t/tau)) = 179
    uint32_t filtered_current_uA_new = (filtered_current_uA * 178 + current_uA)/179;
    uint64_t filtered_current_timestamp_new = time_get_64_isr();
    uint32_t delta_t = filtered_current_timestamp_new - filtered_current_timestamp;
    filtered_current_uA = filtered_current_uA_new;
    filtered_current_timestamp = filtered_current_timestamp_new;

    it_integral_uA_us += (uint64_t) filtered_current_uA_new * (uint64_t) delta_t;
    uint64_t it_integral_decay_uA_us = (uint64_t) IT_INTEGRAL_DECAY_UA * (uint64_t) delta_t;
    if (it_integral_uA_us < it_integral_decay_uA_us) {
        it_integral_uA_us = 0;
    } else {
        it_integral_uA_us -= it_integral_decay_uA_us;
    }
    if (it_integral_uA_us > IT_INTEGRAL_THRESHOLD_UA_US) {
        // it integral threshold reached
        if (state == EndStopState::NONE) {
            uart_writeline("i*t limit reached");
        }
        it_integral_uA_us = 0;
        state = EndStopState::OVERCURRENT;
        end_stop_timestamp = filtered_current_timestamp_new;
    }

    static uint8_t ctr = 0;
    if (++ctr == 20) {
        ctr = 0;
    } else {
        return;
    }

    static uint32_t old_value = 0;
    filtered_current_uA_old = old_value;
    old_value = filtered_current_uA;

    if (filtered_current_uA >= edge_start_uA + EDGE_DETECTION_THRESHOLD_UA)
    {
        // edge detected
        if (state == EndStopState::NONE) {
            uart_writeline("edge detected");
        }
        state = EndStopState::RISING_EDGE;
        end_stop_timestamp = edge_start_timestamp;
        edge_start_uA = filtered_current_uA;
        edge_start_timestamp = filtered_current_timestamp;
        edge_max_uA = filtered_current_uA;
        return;
    }

    if (
        filtered_current_uA - edge_start_uA
        <=
        EDGE_STEEPNESS_THRESHOLD_A_PER_S * (filtered_current_timestamp - edge_start_timestamp)
    ) {
        // the edge was not steep enough, reset its start to here
        edge_start_uA = filtered_current_uA;
        edge_start_timestamp = filtered_current_timestamp;
        edge_max_uA = filtered_current_uA;
    }

    if (filtered_current_uA < filtered_current_uA_old) {
        uint32_t falling_edge_height = edge_max_uA - filtered_current_uA;
        if (falling_edge_height * FALLING_EDGE_HEIGHT_RESET_THRESHOLD_RATIO > (edge_max_uA - edge_start_uA)) {
            edge_max_uA = filtered_current_uA;
            edge_start_uA = filtered_current_uA;
            edge_start_timestamp = filtered_current_timestamp;
        }
    }

    if (filtered_current_uA > edge_max_uA) {
        edge_max_uA = filtered_current_uA;
    }

    uart_write_u16((uint16_t)(filtered_current_uA / 1000));
    uart_write_u16((uint16_t)(it_integral_uA_us / 100000000));
}

EndStopState EndStopDetector::get_end_stop_state(uint64_t *timestamp)
{
    CriticalSectionLock lk;
    auto result = state;

    if (state != EndStopState::NONE) {
        *timestamp = end_stop_timestamp;
        state = EndStopState::NONE;
    }

    return result;
}

void EndStopDetector::clear_end_stop_state()
{
    CriticalSectionLock lk;

    state = EndStopState::NONE;
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
