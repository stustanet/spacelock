#pragma once

#include <cstdint>

extern uint8_t SECRET_KEY[32];

void hmac_update_key(const uint8_t new_val[32]);
