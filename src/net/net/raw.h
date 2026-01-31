#ifndef TINY_NET_ROW_H
#define TINY_NET_ROW_H

#include "sock.h"

typedef struct raw_t
{
    sock_t base;
} raw_t;

net_err_t raw_init();

sock_t* raw_create(int family, int protocol);

#endif //TINY_NET_ROW_H
