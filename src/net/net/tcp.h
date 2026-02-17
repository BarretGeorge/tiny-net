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
            uint16_t f_reserved : 4; // 保留位
            uint16_t f_data_offset : 4; // 数据偏移
            uint16_t f_fin : 1; // 结束标志
            uint16_t f_syn : 1; // 同步标志
            uint16_t f_rst : 1; // 重置标志
            uint16_t f_psh : 1; // 推送标志
            uint16_t f_ack : 1; // 确认标志
            uint16_t f_urg : 1; // 紧急标志
#else
            uint16_t f_data_offset : 4; // 数据偏移
            uint16_t f_reserved : 4; // 保留位
            uint16_t f_urg : 1; // 紧急标志
            uint16_t f_ack : 1; // 确认标志
            uint16_t f_psh : 1; // 推送标志
            uint16_t f_rst : 1; // 重置标志
            uint16_t f_syn : 1; // 同步标志
            uint16_t f_fin : 1; // 结束标志
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

typedef struct tcp_seg_t
{
    ipaddr_t local_ip;
    ipaddr_t remote_ip;
    tcp_header_t* header;
    pktbuf_t* buf;
    uint32_t data_len;
    uint32_t seq;
    uint32_t seq_len;
} tcp_seg_t;

typedef enum tcp_state_t
{
    TCP_STATE_CLOSE,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT,

    TCP_STATE_MAX,
} tcp_state_t;

typedef struct tcp_t
{
    sock_t base;

    tcp_state_t state;

    struct
    {
        uint32_t syn_out : 1;
    } flags;

    struct
    {
        sock_wait_t wait;
    } conn;

    struct
    {
        uint32_t next_seq; // 下一个要发送的序列号
        uint32_t un_ack_seq; // 最后一个未确认的序列号
        uint32_t isn; // 初始序列号
        sock_wait_t wait;
    } send;

    struct
    {
        uint32_t next_seq; // 下一个要接收的序列号
        uint32_t isn; // 初始序列号
        sock_wait_t wait;
    } recv;
} tcp_t;

net_err_t tcp_init(void);

sock_t* tcp_create(int family, int protocol);

size_t tcp_header_size(const tcp_header_t* header);

void tcp_set_header_size(tcp_header_t* header, size_t size);

#endif //TINY_NET_TCP_H
