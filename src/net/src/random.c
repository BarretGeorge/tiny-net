#include "random.h"

static uint64_t s[4];

static uint64_t rotl(const uint64_t x, const int k)
{
    return x << k | x >> (64 - k);
}

void seed_xoshiro(uint64_t seed)
{
    // 用 splitmix64 填充初始状态
    for (int i = 0; i < 4; i++)
    {
        seed += 0x9e3779b97f4a7c15;
        uint64_t z = seed;
        z = (z ^ z >> 30) * 0xbf58476d1ce4e5b9;
        z = (z ^ z >> 27) * 0x94d049bb133111eb;
        s[i] = z ^ z >> 31;
    }
}

uint64_t xoshiro256ss(void)
{
    uint64_t result = rotl(s[1] * 5, 7) * 9;
    uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl(s[3], 45);
    return result;
}

uint64_t random_range(const uint64_t n)
{
    uint64_t mask = ~0ULL >> __builtin_clzll(n | 1); // 找到覆盖n的最小2^k-1
    uint64_t x;
    do
    {
        x = xoshiro256ss() & mask;
    }
    while (x >= n);
    return x;
}
