#pragma once

#include <bitset>
#include <cstdint>

bool dcf77_analyze(const std::bitset<60> &bits, uint8_t bitcount, uint64_t &minute_timestamp);