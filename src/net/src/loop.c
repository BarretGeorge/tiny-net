#include "loop.h"
#include "dbug.h"
#include "netif.h"
#include "ipaddr.h"
#include "exmsg.h"

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
    // 从输出队列获取数据包
    pktbuf_t* buf = netif_get_out(netif, -1);
    if (!buf)
    {
        dbug_error("loop_output: failed to get pktbuf from out_q");
        return NET_ERR_SYS;
    }

    // 直接放回输入队列
    const net_err_t err = netif_put_in(netif, buf, -1);
    if (err != NET_ERR_OK)
    {
        dbug_error("loop_output: failed to put pktbuf to in_q");
        pktbuf_free(buf);
        return err;
    }

    // 通知有数据包到达
    // exmsg_netif_in(netif);
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

    netif_set_addr(netif, &ipaddr, &netmask, NULL);

    netif_set_hwaddr(netif, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 6);

    netif_set_active(netif);

    // netif_set_inactive(netif);
    return NET_ERR_OK;
}
