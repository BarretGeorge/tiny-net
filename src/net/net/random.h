#ifndef TINY_NET_RANDOM_H
#define TINY_NET_RANDOM_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

// 初始化种子
void seed_xoshiro(uint64_t seed);

// 生成随机数
uint64_t xoshiro256ss(void);

// 生成 [0, n) 范围的随机数
uint64_t random_range(uint64_t n);

#endif //TINY_NET_RANDOM_H