#!/usr/bin/env python
# coding=utf-8

# 这是求阶乘最后面有几个零,这是其中一种方式,最简便的方法是利用公式,下次在说
def _fac(n):
    for i in range(1,n):
        n *= i
    return n

def num_of_zero(n):
    count = 0;
    for i in n[::-1]:
        if i == '0':
            count += 1
        else:
            return count

while True:
    try:
        n = int(input())
        n = _fac(n);
        b = str(n);
        count = num_of_zero(b);
        print count
    except:
        break
