#ifndef TINY_NET_ARP_H
#define TINY_NET_ARP_H

#include "ether.h"
#include "ipaddr.h"

// 硬件类型：以太网
#define ARP_HW_ETHER 1
// 操作码：ARP请求
#define ARP_REQUEST 1
// 操作码：ARP应答
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

    // 超时毫秒数
    int timeout;
    // 重试计数
    int retry_cnt;
    nlist_node_t node;
    nlist_t buf_list;
    netif_t* netif;
} arp_entity_t;

// ARP缓存初始化
net_err_t arp_init();

// 发送ARP请求
net_err_t arp_make_request(netif_t* netif, const ipaddr_t* addr);

// 发送无回报ARP请求
net_err_t arp_make_gratuitous_request(netif_t* netif);

// 处理收到的ARP数据包
net_err_t arp_in(netif_t* netif, pktbuf_t* buf);

// 发送ARP应答
net_err_t arp_make_reply(netif_t* netif, pktbuf_t* buf);

// 通过ARP解析MAC地址并发送数据包
net_err_t arp_resolve(netif_t* netif, const ipaddr_t* addr, pktbuf_t* buf);

// 清除ARP缓存
void arp_clear(const netif_t* netif);

// 查找对应IP地址的MAC地址
const uint8_t* arp_find(const netif_t* netif, const ipaddr_t* addr);

#endif //TINY_NET_ARP_H
