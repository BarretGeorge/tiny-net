#ifndef TINY_NET_EXMSG_H
#define TINY_NET_EXMSG_H

#include "net_err.h"
#include "netif.h"
#include "nlist.h"

typedef enum exmsg_type_t
{
    NET_EXMSG_TYPE_INVALID = 0,
    // 数据包到达网卡
    NET_EXMSG_TYPE_NETIF_IN,
} exmsg_type_t;

typedef struct exmsg_netif_t
{
    netif_t* netif;
} exmsg_netif_t;

typedef struct exmsg_t
{
    nlist_node_t node;
    exmsg_type_t type;

    union
    {
        exmsg_netif_t netif;
    };
} exmsg_t;

net_err_t exmsg_init();

net_err_t exmsg_start();

net_err_t exmsg_netif_in(netif_t* netif);

#endif //TINY_NET_EXMSG_H
