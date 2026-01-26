#ifndef TINY_NET_ICMP_V4_H
#define TINY_NET_ICMP_V4_H

#include "net_err.h"
#include "ipaddr.h"
#include "pktbuf.h"

#pragma pack(1)
typedef struct icmp_v4_header_t
{
    // 类型
    uint8_t type;
    // 代码
    uint8_t code;
    // 校验和
    uint16_t checksum;
} icmp_v4_header_t;

typedef struct icmp_v4_pkt_t
{
    icmp_v4_header_t header;

    union
    {
        uint32_t reverse;
    };

    uint8_t payload[0];
} icmp_v4_pkt_t;
#pragma pack()

net_err_t icmp_v4_init(void);

net_err_t icmp_v4_input(const ipaddr_t* src_ip, const ipaddr_t* netif_ip, pktbuf_t* buf);

#endif //TINY_NET_ICMP_V4_H
