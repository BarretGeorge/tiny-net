#include "loop.h"
#include "dbug.h"
#include "netif.h"
#include "ipaddr.h"

static net_err_t loop_open(netif_t* netif, void* data)
{
    netif->type = NETIF_TYPE_LOOPBACK;
    return NET_ERR_OK;
}

static net_err_t loop_close(netif_t* netif)
{
    return NET_ERR_OK;
}

static net_err_t loop_output(netif_t* netif)
{
    return NET_ERR_OK;
}

static netif_open_options_t loop_netif_ops = {
    .open = loop_open,
    .close = loop_close,
    .linkoutput = loop_output,
};

net_err_t loop_init()
{
    netif_t* netif = netif_open("loop", &loop_netif_ops, NULL);
    if (!netif)
    {
        dbug_error("loop_init: failed to open loopback interface");
        return NET_ERR_SYS;
    }

    ipaddr_t ipaddr;
    ipaddr_t netmask;

    ipaddr4_form_str(&ipaddr, "127.0.0.1");
    ipaddr4_form_str(&netmask, "255.0.0.0");

    return NET_ERR_OK;
}
