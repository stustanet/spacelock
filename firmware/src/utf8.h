#pragma once

#include <cstdint>
uint32_t deserialize_utf8(const uint8_t *data, uint16_t *offset, uint8_t increment);
uint8_t get_next_utf8_offset(const uint8_t *data);

