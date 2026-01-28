#ifndef TINY_NET_IPV4_H
#define TINY_NET_IPV4_H

#include <stdint.h>
#include "ipaddr.h"
#include "net_err.h"
#include "netif.h"
#include "pktbuf.h"

#pragma pack(1)
typedef struct ipv4_header_t
{
    union
    {
        struct
        {
#if NET_ENDIAN_LITTLE
            uint16_t shdr : 4; // 头部长度
            uint16_t version : 4; // 版本号
#else
            uint16_t version : 4; // 版本号
            uint16_t shdr : 4; // 头部长度
#endif
            uint16_t tos : 8; // 服务类型
        };

        // 版本和头部长度
        uint16_t shar_all;
    };

    // 总长度
    uint16_t total_len;
    // 标识
    uint16_t id;

    // 标志位和片偏移
    union
    {
        struct
        {
#if NET_ENDIAN_LITTLE
            uint16_t frag_offset : 13; // 片偏移
            uint16_t more_frags : 1; // 更多分片
            uint16_t dont_frag : 1; // 不分片
            uint16_t reserved : 1; // 保留位
#else
            uint16_t reserved : 1; // 保留位
            uint16_t dont_frag : 1; // 不分片
            uint16_t more_frags : 1; // 更多分片
            uint16_t frag_offset : 13; // 片偏移
#endif
        };
        uint16_t frag_all;
    };

    // 生存时间
    uint8_t ttl;
    // 协议类型
    uint8_t protocol;
    // 头部校验和
    uint16_t header_checksum;
    // 源地址
    uint8_t src_addr[IPV4_ADDR_LEN];
    // 目的地址
    uint8_t dest_addr[IPV4_ADDR_LEN];
} ipv4_header_t;

typedef struct ipv4_pkt_t
{
    ipv4_header_t header;
    uint8_t payload[0];
} ipv4_pkt_t;
#pragma pack()

typedef struct ip_fragment_t
{
    ipaddr_t ip;
    uint16_t id;
    int tmo;
    nlist_t buf_list;
    nlist_node_t node;
} ip_fragment_t;

net_err_t ipv4_init();

int ipv4_hdr_size(const ipv4_pkt_t* pkt);

void ipv4_set_hdr_size(ipv4_pkt_t* pkt, int size);

net_err_t ipv4_input(netif_t* netif, pktbuf_t* buf);

net_err_t ipv4_output(uint8_t protocol, const ipaddr_t* dest_ip, const ipaddr_t* src_ip, pktbuf_t* buf);

#endif //TINY_NET_IPV4_H
