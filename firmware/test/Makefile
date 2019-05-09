.PHONY: all
all: sha256test base64test

sha256test: sha256test.cpp sha256.cpp sha256.h Makefile
	g++ -std=c++17 sha256test.cpp sha256.cpp -o sha256test -Wall -Wextra -g

base64test: base64test.cpp base64.c Makefile
	g++ -std=c++17 base64test.cpp base64.c -o base64test -Wall -Wextra -g

.PHONY: run
run: sha256test base64test
	./runtests.py
