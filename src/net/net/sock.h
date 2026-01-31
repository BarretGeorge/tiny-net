#ifndef TINY_NET_SOCK_H
#define TINY_NET_SOCK_H

#include "exmsg.h"
#include "net_err.h"

typedef struct x_socket_t
{
    enum
    {
        SOCK_STATE_FREE = 0,
        SOCK_STATE_USED,
    } state;

    int fd;
    int domain;
    int type;
    int protocol;
} x_socket_t;

typedef struct sock_create_t
{
    int domain;
    int type;
    int protocol;
} sock_create_t;

typedef struct sock_req_t
{
    int fd;

    union
    {
        sock_create_t create;
    };
} sock_req_t;

net_err_t socket_init(void);

net_err_t socket_create_req_in(const func_msg_t* msg);

#endif //TINY_NET_SOCK_H
