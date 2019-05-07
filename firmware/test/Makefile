all: sha256test base64test

sha256test: sha256test.cpp sha256.cpp sha256.h
	g++ -std=c++17 sha256test.cpp sha256.cpp -o sha256test -Wall -Wextra -pedantic

base64test: base64test.cpp base64.c
	g++ -std=c++17 base64test.cpp base64.c -o base64test -Wall -Wextra -pedantic
