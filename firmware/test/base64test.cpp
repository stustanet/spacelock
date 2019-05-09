#include <cstdint>
#include <cstdio>
#include <vector>

#include "base64.h"

const char *HEX_CHARS = "0123456789abcdef";

int main() {
    std::vector<uint8_t> buf;

    while (1) {
        int byte = getchar();
        if (byte < 0) {
            break;
        }
        if (byte != 10) {
            buf.push_back(static_cast<uint8_t>(byte));
        }
    }

    //const char *input = "YWJjZGVmZ2hpCmFiY2RlZmdoaQphYmNkZWZnaGkKYWJjZGVmZ2hpCmFiY2RlZmdoaQphYmNkZWZnCg==";
    //while (*input) { buf.push_back(*(input++)); }

    uint32_t size = base64_decode(buf.data(), buf.size());

    fwrite(buf.data(), size, 1, stdout);

    return 0;
}
