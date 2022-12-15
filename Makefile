CFLAGS=-std=c11 -g -fno-common
CC=clang

rvcc: main.o
	$(CC) -o rvcc $(CFLAGS) main.o 

test: rvcc
	./test.sh

clean:
	rm -rf rvcc *.o, *.s, tmp* a.out

# Indicates that there is no actual dependency file for test and clean
.PHONY: test clean