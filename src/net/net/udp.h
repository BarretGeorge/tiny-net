#ifndef TINY_NET_UDP_H
#define TINY_NET_UDP_H

#include "sock.h"

typedef struct udp_t
{
    sock_t base;
    sock_wait_t recv_wait;
    nlist_t recv_list;
} udp_t;

net_err_t upd_init();

sock_t* udp_create(int family, int protocol);

#endif //TINY_NET_UDP_H
