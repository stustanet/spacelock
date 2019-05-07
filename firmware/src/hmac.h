#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hmac_equals();

constexpr uint32_t HMAC_SIZE = 32;
extern uint8_t SECRET[32];

#ifdef __cplusplus
}
#endif