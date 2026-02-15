#ifndef TINY_NET_TCP_H
#define TINY_NET_TCP_H

#include "sock.h"

#pragma pack(1)
typedef struct tcp_header_t
{
    // 源端口号
    uint16_t src_port;
    // 目标端口号
    uint16_t dest_port;
    // 序列号
    uint32_t seq_num;
    // 确认号
    uint32_t ack_num;

    union
    {
        uint16_t flags;

        struct
        {
#ifdef NET_ENDIAN_LITTLE
            uint16_t reserved : 4; // 保留位
            uint16_t data_offset : 4; // 数据偏移
            uint16_t fin : 1; // 结束标志
            uint16_t syn : 1; // 同步标志
            uint16_t rst : 1; // 重置标志
            uint16_t psh : 1; // 推送标志
            uint16_t ack : 1; // 确认标志
            uint16_t urg : 1; // 紧急标志
#else
            uint16_t data_offset : 4; // 数据偏移
            uint16_t reserved : 4; // 保留位
            uint16_t urg : 1; // 紧急标志
            uint16_t ack : 1; // 确认标志
            uint16_t psh : 1; // 推送标志
            uint16_t rst : 1; // 重置标志
            uint16_t syn : 1; // 同步标志
            uint16_t fin : 1; // 结束标志
#endif
        };
    };

    // 窗口大小
    uint16_t window_size;
    // 校验和
    uint16_t checksum;
    // 紧急指针
    uint16_t urgent_ptr;
} tcp_header_t;

typedef struct tcp_pkt_t
{
    tcp_header_t header;
    uint8_t data[0];
} tcp_pkt_t;
#pragma pack()

typedef struct tcp_t
{
    sock_t base;
} tcp_t;

net_err_t tcp_init(void);

sock_t* tcp_create(int family, int protocol);

size_t tcp_header_size(const tcp_header_t* header);

#endif //TINY_NET_TCP_H
