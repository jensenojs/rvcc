#!/bin/bash

assert() {
    expected="$1"  # expected arg number
    input="$2"     # expected value

    ./rvcc $input > tmp.s || exit
    riscv64-unknown-linux-gnu-gcc -static -o tmp tmp.s

    qemu-riscv64 -L $RISCV/sysroot ./tmp
    actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected, but got $actual"
        exit 1
    fi
}

assert 0 0
assert 11 11

echo OK