#pragma once

#include "stm32f1xx_hal.h"

#ifdef __cplusplus

#include <array>

class CriticalSectionLock {
public:
    CriticalSectionLock() {
        __disable_irq();
    }

    ~CriticalSectionLock() {
        __enable_irq();
    }
};

class UARTRxBuffer {
public:
    std::array<uint8_t, 256> buf;
    uint32_t buf_pos;
    bool finished;

    inline UARTRxBuffer() { this->reset(); }

    inline void reset() {
        this->buf_pos = 0;
        this->finished = false;
    }

    inline void finish() {
        this->finished = true;
    }

    inline void add(uint8_t data) {
        if (this->finished) {
            this->reset();
        }
        if (this->buf_pos >= this->buf.size()) { return; }
        this->buf[this->buf_pos++] = data;
    }
};

class UARTTxBuffer {
public:
    std::array<uint8_t, 256> buf;
    uint32_t start_pos = 0;
    uint32_t end_pos = 0;

    int get_next();
    bool add(uint8_t byte);
    bool add(const char *buf);
    bool add(const uint8_t *buf, uint32_t len);
};

/**
 * Returns nullptr if no new message has been received.
 * Returns UARTRxBuffer containing a message if a new message has been
 * received. The return value is invalidated upon the next call to this
 * function.
 */
UARTRxBuffer *uart_poll_message();

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets a 16-bit timer value in us since boot.
 * Overflows every 65ms.
 */
inline uint16_t time_get_16() {
    return TIM1->CNT;
}

/**
 * Gets a 64-bit timer value in us since boot.
 * Must be called form an ISR, otherwise the
 * behavior is undefined.
 */
uint64_t time_get_64_isr();

/**
 * Gets a 64-bit timer value in us since boot.
 * Overflowsn't.
 */
uint64_t time_get_64();

/**
 * Must be called from the systick handler
 * at least once every 32ms.
 */
void timer_update_extended_bits();

/**
 * To be called from the UART RX interrupt handler
 */
void uart_data_received(uint8_t byte);

/**
 * To be called from the UART TX interrupt handler
 */
void uart_transmit_next();

/**
 * Transmits a line on UART.
 */
void uart_writeline(const char *text);

#ifdef __cplusplus
}
#endif