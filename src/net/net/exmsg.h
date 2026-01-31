#ifndef TINY_NET_EXMSG_H
#define TINY_NET_EXMSG_H

#include "net_err.h"
#include "netif.h"
#include "nlist.h"

struct func_msg_t;

typedef net_err_t (*exmsg_func_t)(const struct func_msg_t* msg);

typedef enum exmsg_type_t
{
    NET_EXMSG_TYPE_INVALID = 0,
    // 数据包到达网卡
    NET_EXMSG_TYPE_NETIF_IN,
    // 用户自定义函数消息
    NET_EXMSG_TYPE_FUNC,
} exmsg_type_t;

typedef struct exmsg_netif_t
{
    netif_t* netif;
} exmsg_netif_t;

typedef struct func_msg_t
{
    sys_thread_t thread;
    exmsg_func_t func;
    void* arg;
    net_err_t err;
    sys_sem_t wait_sem;
} func_msg_t;

typedef struct exmsg_t
{
    nlist_node_t node;
    exmsg_type_t type;

    union
    {
        exmsg_netif_t netif;
        func_msg_t* func;
    };
} exmsg_t;

net_err_t exmsg_init();

net_err_t exmsg_start();

net_err_t exmsg_netif_in(netif_t* netif);

net_err_t exmsg_func_exec(exmsg_func_t func, void* arg);

#endif //TINY_NET_EXMSG_H
