#include "tool.h"

uint16_t checksum16(const void* data, uint16_t size, const uint32_t pre_sum, bool complement)
{
    const uint8_t* bytes = data;
    uint32_t sum = pre_sum;

    // 处理16位数据
    while (size >= 2)
    {
        sum += (bytes[0] << 8) | bytes[1];
        bytes += 2;
        size -= 2;
    }

    // 处理剩余的字节（如果有的话）
    if (size > 0)
    {
        sum += (bytes[0] << 8);
    }

    // 折叠32位和为16位
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t result = (uint16_t)sum;
    if (complement)
    {
        result = ~result;
    }
    return result;
}
