#!/bin/bash

# 校验rvcc生成的汇编能够正确运行的辅助函数
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

# 默认make即make test，展开回归测试

# [1] 支持返回特定数值
assert 42 '{return 42;}'

# [2] 支持+ -运算符
assert 34 '{return 12-34+56;}'

# [3] 支持空白符识别
assert 41 '{return 12 + 34  - 5 ;}'

# [4] 改进报错信息
# 输入./rvcc 1+s 回车，可以观察到结果

# [5] 支持 *， /，() 运算符
assert 47 '{return 5+6*7;}'
assert 77 '{return (5+6)*7;}'
assert 15 '{return 5*(9-6);}'
assert 17 '{return 1-8/(2*2)+3*6;}'

# [6] 支持一元运算符+， -
assert 10 '{ return -10+20; }'
assert 10 '{ return --10; }'
assert 10 '{ return - - +10; }'
assert 48 '{ return ------12*+++++----++++++++++4; }'

# [7] 支持 == != <= >=
assert 0 '{ return 0==1; }'
assert 1 '{ return 42==42; }'
assert 1 '{ return 0!=1; }'
assert 0 '{ return 42!=42; }'
assert 1 '{ return 0<1; }'
assert 0 '{ return 1<1; }'
assert 0 '{ return 2<1; }'
assert 1 '{ return 0<=1; }'
assert 1 '{ return 1<=1; }'
assert 0 '{ return 2<=1; }'
assert 1 '{ return 1>0; }'
assert 0 '{ return 1>1; }'
assert 0 '{ return 1>2; }'
assert 1 '{ return 1>=0; }'
assert 1 '{ return 1>=1; }'
assert 0 '{ return 1>=2; }'
assert 1 '{ return 5==2+3; }'
assert 0 '{ return 6==4+3; }'
assert 1 '{ return 0*9+5*2==4+4*(6/3)-2; }'

#[8] 将main.c拆分成多个文件

#[9]  支持 ; 分割语句

# [10] 支持单字母变量
assert 3 '{ int a=3; return a; }'
assert 5 '{ int a=3, z=5; return z; }'
assert 8 '{ int a=3, z=5; return a+z; }'
assert 6 '{ int a,b; a=b=3; return a+b; }'
assert 5 '{ int a=3,b=4,a=1;return a+b; }'

# [11] 支持多字母变量
assert 3 '{int foo=3; return foo;}'
assert 74 '{int foo2=70;int bar4=4; return foo2+bar4;}'

# [12] 支持return语句
assert 1 '{return 1; 2; 3;}'
assert 2 '{1; return 2; 3;}'
assert 3 '{1; 2; return 3;}'

# [13] 支持 { ... }

# [14] 支持空语句
assert 5 '{ ;;;  return 5;}'

# [15] 支持if语句
assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

# [16] 支持for语句
assert 55 '{int i = 0;int j = 0; for(i = 0; i <= 10; i=i+1) j = i + j; return j; }'
assert 3 '{ for (;;) {return 3;} return 5; }'

# [17] 支持while语句
assert 10 '{int i=0; while(i<10) { i=i+1; } return i; }'

# [18] 更新辅助信息(.s生成的注释)

# [19] 为节点添加相应的终结符，以改进报错信息

# [20] 支持一元& *运算符
assert 3 '{ int x=3; return *&x; }'
assert 3 '{ int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5 '{ int x=3; int *y=&x; *y=5; return x; }'

# [21] 支持指针的算术运算
assert 3 '{ int x=3; int y=5; return *(&y-1); }'
assert 5 '{ int x=3; int y=5; return *(&x+1); }'
assert 7 '{ int x=3; int y=5; *(&y-1)=7; return x; }'
assert 7 '{ int x=3; int y=5; *(&x+1)=7; return y; }'

# [22] 支持int关键字
assert 8 '{ int x, y; x=3; y=5; return x+y; }'
assert 8 '{ int x=3, y=5; return x+y; }'

echo OK