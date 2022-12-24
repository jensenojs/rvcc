CFLAGS=-std=c11 -g -fno-common -Wall -Werror
CC=clang

test: rvcc
	./test.sh

rvcc: main.o
	$(CC) -o rvcc $(CFLAGS) main.o 

clean:
	rm -rf rvcc *.o, *.s, tmp* a.out

# Indicates that there is no actual dependency file for test and clean
.PHONY: test clean