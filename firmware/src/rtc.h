#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

#define RTC_ADDR 0b1101000<<1 //last bit for R/W


uint64_t read_rtc(void);
uint8_t write_rtc(uint8_t years, uint8_t months, uint8_t weekday, uint8_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);

#ifdef __cplusplus
}
#endif
