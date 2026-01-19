#ifndef TINY_NET_ARP_H
#define TINY_NET_ARP_H

#include "ether.h"
#include "ipaddr.h"

#define ARP_HW_ETHER 1
#define ARP_REQUEST 1
#define ARP_REPLY 2

#pragma pack(1)
typedef struct arp_pkt_t
{
    // 硬件类型
    uint16_t h_type;
    // 协议类型
    uint16_t p_type;
    // 硬件地址长度
    uint8_t hw_len;
    // 协议地址长度
    uint8_t p_len;
    // 操作码
    uint16_t opcode;
    // 发送方mac地址
    uint8_t sender_hwaddr[ETHER_HWADDR_LEN];
    // 发送方IP地址
    uint8_t sender_addr[IPV4_ADDR_LEN];
    // 接收方mac地址
    uint8_t target_hwaddr[ETHER_HWADDR_LEN];
    // 接收方IP地址
    uint8_t target_addr[IPV4_ADDR_LEN];
} arp_pkt_t;
#pragma pack()

typedef struct arp_entity_t
{
    uint8_t p_addr[IPV4_ADDR_LEN];
    uint8_t hwaddr[ETHER_HWADDR_LEN];

    enum
    {
        NET_ARP_FREE,
        NET_ARP_WAITING,
        NET_ARP_RESOLVE
    } state;

    nlist_node_t node;
    nlist_t buf_list;
    netif_t* netif;
} arp_entity_t;

net_err_t arp_init();

net_err_t arp_make_request(netif_t* netif, const ipaddr_t* addr);

#endif //TINY_NET_ARP_H
