#include "deserialize.h"

uint64_t deserialize_u64(const uint8_t *buf) {
    return (
        (static_cast<uint64_t>(buf[0]) <<  0) |
        (static_cast<uint64_t>(buf[1]) <<  8) |
        (static_cast<uint64_t>(buf[2]) << 16) |
        (static_cast<uint64_t>(buf[3]) << 24) |
        (static_cast<uint64_t>(buf[4]) << 32) |
        (static_cast<uint64_t>(buf[5]) << 40) |
        (static_cast<uint64_t>(buf[6]) << 48) |
        (static_cast<uint64_t>(buf[7]) << 56)
    );
}

uint32_t deserialize_u32(const uint8_t *buf) {
    return (
        (static_cast<uint32_t>(buf[0]) <<  0) |
        (static_cast<uint32_t>(buf[1]) <<  8) |
        (static_cast<uint32_t>(buf[2]) << 16) |
        (static_cast<uint32_t>(buf[3]) << 24)
    );
}