#include <cstdint>

class SHA256 {
public:
    SHA256();
    void reset();
    void add_byte(const uint8_t byte);
    void update(const uint8_t *buf, uint32_t size);
    void calculate_digest(uint8_t *hash);

private:
    // internal state
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];

    void transform();
};