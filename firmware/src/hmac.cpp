#include "hmac.h"

#include "secret_key.h"
#include "sha256.h"

#define OPAD 0x5c
#define IPAD 0x36

#define BLOCK_SIZE 64

void get_padded_key(uint8_t padding, uint8_t result[BLOCK_SIZE]);

void get_padded_key(uint8_t padding, uint8_t result[BLOCK_SIZE]) {
    uint32_t i = 0;
    for (; i < sizeof(SECRET_KEY); i++) {
        result[i] = SECRET_KEY[i] ^ padding;
    }
    for (; i < BLOCK_SIZE; i++) {
        result[i] = padding;
    }
}

void hmac(const uint8_t *data, uint32_t len, uint8_t result[32])
{
    uint8_t key_padded[64];

    // calculate the inner hash
    get_padded_key(IPAD, key_padded);
    SHA256 hash;
    hash.update(key_padded, sizeof(key_padded));
    hash.update(data, len);
    hash.calculate_digest(result);
    hash.reset();

    // calculate the outer hash
    get_padded_key(OPAD, key_padded);
    hash.update(key_padded, sizeof(key_padded));
    hash.update(result, 32);
    hash.calculate_digest(result);
}
