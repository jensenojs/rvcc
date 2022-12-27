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
assert 42 '42;'

# [2] 支持+ -运算符
assert 34 '12-34+ 56;'

# [3] 支持空白符识别
assert 41 '12 + 34  - 5 ;'

# [4] 改进报错信息
# 输入./rvcc 1+s 回车，可以观察到结果

# [5] 支持 *， /，() 运算符
assert 47 '5+6*7;'
assert 77 '(5+6)*7;'
assert 15 '5*(9-6);'
assert 17 '1-8/(2*2)+3*6;'

# [6] 支持一元运算符+， -
assert 10 '-10+20;'
assert 10 '--10;'
assert 10 '- - +10;'
assert 48 '------12*+++++----++++++++++4;'

# [7] 支持 == != <= >=
assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'
assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'
assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'
assert 1 '5==2+3;'
assert 0 '6==4+3;'
assert 1 '0*9+5*2==4+4*(6/3)-2;'

#[8] 将main.c拆分成多个文件

#[9]  支持 ; 分割语句

# [10] 支持单字母变量
assert 3 'a=3; a;'
assert 5 'a=3; z=5; z;'
assert 8 'a=3; z=5; a+z;'
assert 6 'a=b=3; a+b;'
assert 5 'a=3;b=4;a=1;a+b;'

# [11] 支持多字母变量
assert 3 'foo=3; foo;'
assert 74 'foo2=70; bar4=4; foo2+bar4;'

echo OK