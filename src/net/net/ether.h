#ifndef TINY_NET_ETHER_H
#define TINY_NET_ETHER_H

#include "net_err.h"
#include <stdint.h>

#include "netif.h"
#include "pktbuf.h"

// 以太网类型定义
#define ETHER_TYPE_IPv4 0x0800

// 以太网硬件地址长度
#define ETHER_HWADDR_LEN 6

// 以太网帧负载最大长度
#define ETHER_PAYLOAD_MAX_LEN 1500

// 以太网帧负载最小长度
#define ETHER_PAYLOAD_MIN_LEN 46

#pragma pack(1)
typedef struct ether_header_t
{
    // 目的MAC地址
    uint8_t dest_mac[ETHER_HWADDR_LEN];
    // 源MAC地址
    uint8_t src_mac[ETHER_HWADDR_LEN];
    // 以太网协议类型
    uint16_t protocol;
} ether_header_t;
#pragma pack()

#pragma pack(1)
typedef struct ether_frame_t
{
    // 以太网帧头
    ether_header_t header;
    // 以太网帧负载
    uint8_t payload[ETHER_PAYLOAD_MAX_LEN];
} ether_frame_t;
#pragma pack()

net_err_t ether_init();

const uint8_t* ether_broadcast_addr();

net_err_t ether_raw_out(netif_t* netif, uint16_t protocol, const uint8_t* dest_mac, pktbuf_t* buf);

#endif //TINY_NET_ETHER_H
