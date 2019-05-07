#include "sha256.h"

constexpr uint32_t rotate_left(uint32_t a, uint32_t b) {
    return (((a) << (b)) | ((a) >> (32-(b))));
}

constexpr uint32_t rotate_right(uint32_t a, uint32_t b) {
    return (((a) >> (b)) | ((a) << (32-(b))));
}

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)  (rotate_right(x,  2) ^ rotate_right(x, 13) ^ rotate_right(x, 22))
#define EP1(x)  (rotate_right(x,  6) ^ rotate_right(x, 11) ^ rotate_right(x, 25))
#define SIG0(x) (rotate_right(x,  7) ^ rotate_right(x, 18) ^ ((x) >>  3))
#define SIG1(x) (rotate_right(x, 17) ^ rotate_right(x, 19) ^ ((x) >> 10))

static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


SHA256::SHA256() {
    this->reset();
}


void SHA256::transform() {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (this->data[j] << 24) | (this->data[j + 1] << 16) | (this->data[j + 2] << 8) | (this->data[j + 3]);
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = this->state[0];
    b = this->state[1];
    c = this->state[2];
    d = this->state[3];
    e = this->state[4];
    f = this->state[5];
    g = this->state[6];
    h = this->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    this->state[0] += a;
    this->state[1] += b;
    this->state[2] += c;
    this->state[3] += d;
    this->state[4] += e;
    this->state[5] += f;
    this->state[6] += g;
    this->state[7] += h;
}

void SHA256::reset() {
    this->datalen = 0;
    this->bitlen = 0;
    this->state[0] = 0x6a09e667;
    this->state[1] = 0xbb67ae85;
    this->state[2] = 0x3c6ef372;
    this->state[3] = 0xa54ff53a;
    this->state[4] = 0x510e527f;
    this->state[5] = 0x9b05688c;
    this->state[6] = 0x1f83d9ab;
    this->state[7] = 0x5be0cd19;
}

void SHA256::update(const uint8_t *buf, uint32_t bufsize) {
    for (uint32_t i = 0; i < bufsize; i++) {
        this->add_byte(buf[i]);
    }
}

void SHA256::add_byte(const uint8_t byte) {
    this->data[this->datalen++] = byte;

    if (this->datalen == 64) {
        this->transform();
        this->bitlen += 512;
        this->datalen = 0;
    }
}

void SHA256::calculate_digest(uint8_t *digest) {
    uint32_t i = this->datalen;

    // Pad whatever data is left in the buffer.
    if (this->datalen < 56) {
        this->data[i++] = 0x80;
        while (i < 56) {
            this->data[i++] = 0x00;
        }
    } else {
        this->data[i++] = 0x80;
        while (i < 64) {
            this->data[i++] = 0x00;
        }

        this->transform();

        for (i = 0; i < 56; i++) {
            this->data[i] = 0;
        }
    }

    // Append to the padding the total message's length in bits and transform.
    this->bitlen += this->datalen * 8;
    this->data[63] = this->bitlen;
    this->data[62] = this->bitlen >> 8;
    this->data[61] = this->bitlen >> 16;
    this->data[60] = this->bitlen >> 24;
    this->data[59] = this->bitlen >> 32;
    this->data[58] = this->bitlen >> 40;
    this->data[57] = this->bitlen >> 48;
    this->data[56] = this->bitlen >> 56;

    this->transform();

    // Since this implementation uses little endian byte ordering and SHA uses big endian,
    // reverse all the bytes when copying the final state to the output hash.
    for (i = 0; i < 4; ++i) {
        digest[i +  0] = static_cast<uint8_t>(this->state[0] >> (24 - i * 8));
        digest[i +  4] = static_cast<uint8_t>(this->state[1] >> (24 - i * 8));
        digest[i +  8] = static_cast<uint8_t>(this->state[2] >> (24 - i * 8));
        digest[i + 12] = static_cast<uint8_t>(this->state[3] >> (24 - i * 8));
        digest[i + 16] = static_cast<uint8_t>(this->state[4] >> (24 - i * 8));
        digest[i + 20] = static_cast<uint8_t>(this->state[5] >> (24 - i * 8));
        digest[i + 24] = static_cast<uint8_t>(this->state[6] >> (24 - i * 8));
        digest[i + 28] = static_cast<uint8_t>(this->state[7] >> (24 - i * 8));
    }
}
