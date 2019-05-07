#include "base64.h"

static const uint8_t base64_index[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0, 62, 63, 62, 62, 63,
   52, 53, 54, 55, 56, 57, 58, 59,
   60, 61,  0,  0,  0,  0,  0,  0,
    0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22,
   23, 24, 25,  0,  0,  0,  0, 63,
    0, 26, 27, 28, 29, 30, 31, 32,
   33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48,
   49, 50, 51,  0,  0,  0,  0,  0
};

uint32_t base64_decode(uint8_t *buf, const uint32_t bufsize)
{
    uint8_t *p = (uint8_t *)buf;
    uint32_t pad = bufsize > 0 && (bufsize % 4 || p[bufsize - 1] == '=');

    const uint32_t L = ((bufsize + 3) / 4 - pad) * 4;
    uint32_t resultsize = L / 4 * 3 + pad;

    for (uint32_t i = 0, j = 0; i < L; i += 4)
    {
        int n = (
            (base64_index[p[i + 0]] << 18) |
            (base64_index[p[i + 1]] << 12) |
            (base64_index[p[i + 2]] <<  6) |
            (base64_index[p[i + 3]] <<  0)
        );

        buf[j++] = (uint8_t)(n >> 16);
        buf[j++] = (uint8_t)(n >>  8);
        buf[j++] = (uint8_t)(n >>  0);
    }

    if (pad)
    {
        int n = (
            (base64_index[p[L + 0]] << 18) |
            (base64_index[p[L + 1]] << 12)
        );

        buf[resultsize - 1] = (uint8_t)(n >> 16);

        if ((bufsize > (L + 2)) && (p[L + 2] != '='))
        {
            n |= (base64_index[p[L + 2]] << 6);
            buf[resultsize] = (uint8_t)(n >> 8);
            resultsize++;
        }
    }

    return resultsize;
}