CFLAGS=-std=c11 -g -fno-common -Wall -Werror
CC=clang
# Represents all ".c" terminated files
SRCS=$(wildcard *.c)
# replacing all .c files with filenames ending in .o of the same name
OBJS=$(SRCS:.c=.o)

test: rvcc
	./test.sh

rvcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# All relocatable files depend on the rvcc.h header file
$(OBJS): rvcc.h

clean:
	rm -rf rvcc *.o *.s tmp* a.out

# Indicates that there is no actual dependency file for test and clean
.PHONY: test clean