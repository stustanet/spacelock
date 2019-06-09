#include <array>
#include <iostream>
#include <iterator>
#include <string>

#include "hmac.h"
#include "secret_key.h"


int main() {
	std::istreambuf_iterator<char> begin{std::cin}, end;
	std::string input{begin, end};

	// the first 32 bytes are the hmac key
	if (input.size() < 32) {
		std::cout << "not enough input" << std::endl;
		return 1;
	}

	std::string key = input.substr(0, 32);
	std::string data = input.substr(32);

	std::array<uint8_t, 32> digest;

	hmac_update_key(reinterpret_cast<const uint8_t *>(key.c_str()));
	hmac(reinterpret_cast<const uint8_t*>(data.c_str()), data.size(), digest.data());

	std::copy(std::begin(digest), std::end(digest), std::ostream_iterator<uint8_t>(std::cout));
	std::cout.flush();
	return 0;
}
