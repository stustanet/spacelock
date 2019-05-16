.PHONY: all
all: sha256test base64test gregoriancalendartest

sha256test: sha256test.cpp sha256.cpp sha256.h Makefile
	g++ -std=c++17 sha256test.cpp sha256.cpp -o sha256test -Wall -Wextra -g

base64test: base64test.cpp base64.c base64.h Makefile
	g++ -std=c++17 base64test.cpp base64.c -o base64test -Wall -Wextra -g

gregoriancalendartest: gregoriancalendartest.cpp gregorian_calendar.cpp gregorian_calendar.h Makefile
	g++ -std=c++17 gregoriancalendartest.cpp gregorian_calendar.cpp -o gregoriancalendartest -Wall -Wextra -g

.PHONY: run
run: sha256test base64test gregoriancalendartest
	./runtests.py
