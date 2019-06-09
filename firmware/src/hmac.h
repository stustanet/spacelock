#pragma once

#include <cstdint>

#ifndef __cplusplus
#error "this header file cannot be included in C mode"
#endif

void hmac(const uint8_t *data, uint32_t len, uint32_t result[32]);