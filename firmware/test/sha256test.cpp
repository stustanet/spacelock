#include <array>
#include <cstdio>

#include "sha256.h"

const char *HEX_CHARS = "0123456789abcdef";

int main() {
    SHA256 hash;
    std::array<uint8_t, 32> digest;

    while (1) {
        int byte = getchar();
        if (byte < 0) {
            break;
        }

        hash.add_byte(static_cast<uint8_t>(byte));
    }

    hash.calculate_digest(digest.data());

    for (uint8_t byte : digest) {
        putchar(HEX_CHARS[byte >> 4]);
        putchar(HEX_CHARS[byte & 0xf]);
    }

    putchar('\n');

    return 0;
}
