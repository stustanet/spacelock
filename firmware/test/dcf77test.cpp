#include <bitset>
#include <cstdint>
#include <cstdio>

#include "dcf77_analyze.h"

int main() {
    uint8_t bitcount;
    std::bitset<60> bits;
    uint64_t timestamp;

    while (1) {
        bitcount = 0;
        while (1) {
            char c = getchar();
            if (c == ' ') { continue; }
            if (c == '\n') { break; }
            if (bitcount >= 60) { return 1; }
            if (c == '0') { bits[bitcount++] = 0; continue; }
            if (c == '1') { bits[bitcount++] = 1; continue; }
            return 0;
        }

        if (!dcf77_analyze(bits, bitcount, timestamp)) {
            printf("fail\n");
        } else {
            printf("%ld\n", timestamp);
        }
    }

    return 0;
}
