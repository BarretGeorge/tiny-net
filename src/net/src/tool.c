#include "tool.h"

uint16_t checksum16(const void* data, uint16_t size, const uint32_t pre_sum, bool complement)
{
    uint16_t* curr_buf = (uint16_t*)data;
    uint32_t checksum = pre_sum;

    while (size > 1)
    {
        checksum += *curr_buf++;
        size -= 2;
    }

    if (size > 0)
    {
        checksum += *(uint8_t*)curr_buf;
    }

    uint16_t high;
    while ((high = checksum >> 16) != 0)
    {
        checksum = high + (checksum & 0xffff);
    }

    return complement ? (uint16_t)~checksum : (uint16_t)checksum;
}
