#ifndef TINY_NET_PROTOCOL_H
#define TINY_NET_PROTOCOL_H

typedef enum protocol_type_t
{
    PROTOCOL_TYPE_NONE = 0,
    // ARP 协议
    PROTOCOL_TYPE_ARP = 0x0806,
    // IPv4 协议
    PROTOCOL_TYPE_IPv4 = 0x0800,
    // IPv6 协议
    PROTOCOL_TYPE_IPv6 = 0x86DD,
} protocol_type_t;

#endif //TINY_NET_PROTOCOL_H
