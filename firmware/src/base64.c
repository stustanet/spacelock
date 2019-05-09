#include "base64.h"

/**
 * buf[:(bufsize/4)*4+4] must be readable and writable,
 * and buf[:bufsize] must contain valid data,
 * otherwise the behavior is undefined.
 * the result will be written to buf, and the result size returned.
 * the remainder of buf will have undefined content.
 */
uint32_t base64_decode(uint8_t *buf, uint32_t bufsize) {
    // fix the padding
    while (bufsize % 4) { buf[bufsize++] = '='; }

    uint8_t *result = buf;
    uint32_t resultsize = 0;
    uint8_t *end = &buf[bufsize];

    while (buf != end) {
        // decode one quadruplet, write one triplet to result
        uint32_t word_value = 0;
        uint32_t word_chars = 3;

        for (uint32_t i = 0; i < 4; i++) {
            uint32_t value = 0;

            switch (buf[i]) {
            case 'A' ... 'Z': value =  0 + buf[i] - 'A'; break;
            case 'a' ... 'z': value = 26 + buf[i] - 'a'; break;
            case '0' ... '9': value = 52 + buf[i] - '0'; break;
            case '+':         value = 62;                break;
            case '/':         value = 63;                break;
            case '=':
                {
                    // '=' is only allowed as the padding in the last two bytes.
                    if (i == 2) {
                        if (buf[i + 1] != '=') { return 0; }
                        word_chars = 1;
                        i = 3;
                    } else if (i == 3) {
                        word_chars = 2;
                    } else {
                        return 0;
                    }
                }
                break;
            default:          return 0;
            }

            word_value |= (value << (6 * (3 - i)));
        }

        result[resultsize++] = (uint8_t)(word_value >> 16);
        if (word_chars > 1) {
            result[resultsize++] = (uint8_t)(word_value >> 8);
        }
        if (word_chars > 2) {
            result[resultsize++] = (uint8_t)(word_value);
        }
        buf += 4;
    }

    return resultsize;
}
