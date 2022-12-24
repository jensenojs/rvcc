#!/bin/bash

assert() {
    expected="$1"  # expected arg number
    input="$2"     # argument sent to rvcc

    ./rvcc "$input" > tmp.s || exit # "$input" but not $input
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

# [1] 支持返回特定数值
assert 42 42

# [2] 支持+ -运算符
assert 34 '12-34+ 56'

# [3] 支持空白符识别
assert 41 '12 + 34  - 5 '

# [4] 改进报错信息
# 输入./rvcc 1+s 回车，可以观察到结果

# [5] 支持 *， /，() 运算符
assert 47 '5+6*7'
assert 77 '(5+6)*7'
assert 15 '5*(9-6)'
assert 17 '1-8/(2*2)+3*6'

# [6] 支持一元运算符+， -
assert 10 '-10+20'
assert 10 '--10'
assert 10 '- - +10'
assert 48 '------12*+++++----++++++++++4'


echo OK