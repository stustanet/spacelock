#pragma once

#include <cstdint>

extern const uint8_t SECRET_KEY[32];

bool secret_key_write(uint8_t secret_key[32]);
