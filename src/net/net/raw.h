#ifndef TINY_NET_ROW_H
#define TINY_NET_ROW_H

#include "sock.h"
#include "nlist.h"

typedef struct raw_t
{
    sock_t base;
    sock_wait_t recv_wait;
    nlist_t recv_list;
} raw_t;

net_err_t raw_init();

sock_t* raw_create(int family, int protocol);

net_err_t raw_input(pktbuf_t* pktbuf);

#endif //TINY_NET_ROW_H
