#ifndef TINY_NET_TOOL_H
#define TINY_NET_TOOL_H

#include <stdbool.h>
#include <stdint.h>
#include "net_cfg.h"
#include "ipaddr.h"
#include "pktbuf.h"

static inline uint16_t swap_u16(const uint16_t val)
{
    return (val << 8) | (val >> 8);
}

static inline uint32_t swap_u32(const uint32_t val)
{
    return ((val >> 24) & 0x000000FF) |
        ((val >> 8) & 0x0000FF00) |
        ((val << 8) & 0x00FF0000) |
        ((val << 24) & 0xFF000000);
}

static inline uint32_t host_to_net_u32(const uint32_t val)
{
#if NET_ENDIAN_LITTLE
    return swap_u32(val);
#else
    return val;
#endif
}

static inline uint32_t net_to_host_u32(const uint32_t val)
{
#if NET_ENDIAN_LITTLE
    return swap_u32(val);
#else
    return val;
#endif
}

static inline uint16_t host_to_net_u16(const uint16_t val)
{
#if NET_ENDIAN_LITTLE
    return swap_u16(val);
#else
    return val;
#endif
}

static inline uint16_t net_to_host_u16(const uint16_t val)
{
#if NET_ENDIAN_LITTLE
    return swap_u16(val);
#else
    return val;
#endif
}

#define x_htons host_to_net_u16
#define x_ntohs net_to_host_u16
#define x_htonl host_to_net_u32
#define x_ntohl net_to_host_u32

// 计算16位校验和
uint16_t checksum16(const void* data, uint16_t size, uint32_t pre_sum, bool complement);

// 计算伪首部校验和
uint16_t checksum16_pseudo(pktbuf_t* buf, const ipaddr_t* src_ip, const ipaddr_t* dst_ip, uint8_t protocol);

#endif //TINY_NET_TOOL_H
