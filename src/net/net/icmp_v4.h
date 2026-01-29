#ifndef TINY_NET_ICMP_V4_H
#define TINY_NET_ICMP_V4_H

#include "net_err.h"
#include "ipaddr.h"
#include "pktbuf.h"

typedef enum icmp_v4_type_t
{
    ICMP_V4_TYPE_ECHO_REPLY = 0, // 回显应答
    ICMP_V4_TYPE_ECHO_REQUEST = 8, // 回显请求
    ICMP_V4_TYPE_UNREACH = 3, // 目的不可达
    ICMP_V4_TYPE_TIME_EXCEEDED = 11, // 超时
} icmp_v4_type_t;

typedef enum icmp_v4_code_t
{
    ICMP_V4_CODE_UNREACH_NET = 0, // 网络不可达
    ICMP_V4_CODE_UNREACH_HOST = 1, // 主机不可达
    ICMP_V4_CODE_UNREACH_PROTOCOL = 2, // 协议不可达
    ICMP_V4_CODE_UNREACH_PORT = 3, // 端口不可达
    ICMP_V4_CODE_TIME_EXCEEDED_TTL = 0, // 生存时间超时
    ICMP_V4_CODE_TIME_EXCEEDED_FRAG = 1, // 分片重组超时
} icmp_v4_code_t;

#pragma pack(1)
typedef struct icmp_v4_header_t
{
    // 类型
    uint8_t type;
    // 代码
    uint8_t code;
    // 校验和
    uint16_t checksum;

    union
    {
        uint32_t unused;

        struct
        {
            uint16_t id; // 标识符
            uint16_t seq; // 序列号
        } echo;

        uint32_t reverse;
    };
} icmp_v4_header_t;

typedef struct icmp_v4_pkt_t
{
    icmp_v4_header_t header;
    uint8_t payload[0];
} icmp_v4_pkt_t;
#pragma pack()

// icmp 模块初始化
net_err_t icmp_v4_init(void);

// 处理输入的 icmpv4 数据包
net_err_t icmp_v4_input(const ipaddr_t* src_ip, const ipaddr_t* netif_ip, pktbuf_t* buf);

// 发送不可达错误回应
net_err_t icmp_v4_output_unreach(const ipaddr_t* dest_ip, const ipaddr_t* src_ip, uint8_t code, pktbuf_t* ip_buf);

#endif //TINY_NET_ICMP_V4_H
