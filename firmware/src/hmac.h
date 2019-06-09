#pragma once

#include <cstdint>

#ifndef __cplusplus
#error "this header file cannot be included in C mode"
#endif

#define HMAC_SIZE 16

void hmac(const uint8_t *data, uint32_t len, uint8_t result[32]);