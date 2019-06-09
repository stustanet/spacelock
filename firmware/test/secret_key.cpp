#include "secret_key.h"

#include <cstring>

uint8_t SECRET_KEY[32] = {0};

void hmac_update_key(const uint8_t new_val[32]) {
	std::memcpy(SECRET_KEY, new_val, 32);
}
