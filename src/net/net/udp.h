#ifndef TINY_NET_UDP_H
#define TINY_NET_UDP_H

#include "sock.h"

#pragma pack(1)
typedef struct udp_header_t
{
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} udp_header_t;

typedef struct udp_pkt_t
{
    udp_header_t header;
    uint8_t data[0];
} udp_packet_t;
#pragma pack()

typedef struct udp_t
{
    sock_t base;
    sock_wait_t recv_wait;
    nlist_t recv_list;
} udp_t;

net_err_t upd_init();

sock_t* udp_create(int family, int protocol);

net_err_t upd_output(const ipaddr_t* dest_ip, uint16_t dest_port, const ipaddr_t* src_ip, uint16_t src_port, pktbuf_t* buf);

#endif //TINY_NET_UDP_H
