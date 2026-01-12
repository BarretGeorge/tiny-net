#include "netif.h"
#include "mblock.h"
#include "pktbuf.h"
#include "dbug.h"
#include "ether.h"
#include "exmsg.h"
#include "protocol.h"

// 网卡接口内存块
static netif_t netif_buffer[NETIF_DEV_CNT];

// 网卡接口内存块管理结构体
static mblock_t netif_mblock;

// 网卡接口链表
static nlist_t netif_list;

// 默认网卡接口
static netif_t* netif_default;

// 链路层指针数组
static const link_layer_t* link_layers[NETIF_TYPE_SIZE];

#if DBG_DISPLAY_ENABLE(DBG_BUG)
static void display_netif_list()
{
    plat_printf("netif list:\n");

    nlist_node_t* node;

    nlist_for_each(node, &netif_list)
    {
        netif_t* netif = nlist_entry(node, netif_t, node);
        plat_printf("  %s", netif->name);
        switch (netif->state)
        {
        case NETIF_STATE_OPENED:
            plat_printf(" (OPENED) ");
            break;
        case NETIF_STATE_ACTIVE:
            plat_printf(" (ACTIVE) ");
            break;
        case NETIF_STATE_CLOSED:
            plat_printf(" (CLOSED) ");
            break;
        default:
            plat_printf(" (UNKNOWN) ");
            break;
        }

        switch (netif->type)
        {
        case NETIF_TYPE_ETHERNET:
            plat_printf(" Type: ETHERNET ");
            break;
        case NETIF_TYPE_WIFI:
            plat_printf(" Type: WIFI ");
            break;
        case NETIF_TYPE_LOOPBACK:
            plat_printf(" Type: LOOPBACK ");
            break;
        default:
            plat_printf(" Type: NONE ");
            break;
        }

        plat_printf("  mtu: %d\n", netif->mtu);

        plat_printf("  hwaddr:");
        dbug_dump_hwaddr(netif->hwaddr.addr, netif->hwaddr.len);
        plat_printf("  ipaddr:");
        dbug_dump_ipaddr(&netif->ipaddr);
        plat_printf("  netmask:");
        dbug_dump_ipaddr(&netif->netmask);
        plat_printf("  gateway:");
        dbug_dump_ipaddr(&netif->gateway);
    }
}
#else
#define display_netif_list()
#endif


net_err_t netif_init()
{
    dbug_info("init netif");

    netif_default = NULL;

    // 初始化网卡链表
    nlist_init(&netif_list);

    // 初始化网卡接口内存块
    mblock_init(&netif_mblock, netif_buffer, sizeof(netif_t), NETIF_DEV_CNT, NLOCKER_TYPE_NONE);

    // 初始化链路层指针数组
    plat_memset(link_layers, 0, sizeof(link_layers));
    return NET_ERR_OK;
}

static const link_layer_t* netif_get_link_layer(const netif_type_t type)
{
    if (type <= NETIF_TYPE_NONE || type >= NETIF_TYPE_SIZE)
    {
        return NULL;
    }
    return link_layers[type - 1];
}

netif_t* netif_open(const char* dev_name, const netif_open_options_t* opts, void* opts_data)
{
    netif_t* netif = mblock_alloc(&netif_mblock, 0);
    if (!netif)
    {
        dbug_error("no mem for netif");
        return NULL;
    }

    ipaddr_set_any(&netif->ipaddr);
    ipaddr_set_any(&netif->netmask);
    ipaddr_set_any(&netif->gateway);

    netif->mtu = 0;
    netif->type = NETIF_TYPE_NONE;

    plat_strncpy(netif->name, dev_name, NETIF_NAME_LEN);
    netif->name[NETIF_NAME_LEN - 1] = '\0';

    plat_memset(&netif->hwaddr, 0, sizeof(netif_hwaddr_t));

    nlist_node_init(&netif->node);

    net_err_t err = fixq_init(&netif->in_q, netif->in_q_buf, NETIF_IN_QUEUE_SIZE, NLOCKER_TYPE_THREAD);
    if (err != NET_ERR_OK)
    {
        dbug_error("init netif in_q failed");
        goto open_failed;
    }

    err = fixq_init(&netif->out_q, netif->out_q_buf, NETIF_OUT_QUEUE_SIZE, NLOCKER_TYPE_THREAD);
    if (err != NET_ERR_OK)
    {
        dbug_error("init netif out_q failed");
        goto open_failed;
    }

    netif->opts = (netif_open_options_t*)opts;
    netif->opts_data = opts_data;

    err = opts->open(netif, opts_data);
    if (err != NET_ERR_OK)
    {
        dbug_error("open netif %s failed", dev_name);
        goto open_failed;
    }
    netif->state = NETIF_STATE_OPENED;

    if (netif->type == NETIF_TYPE_NONE)
    {
        dbug_error("netif %s type invalid after open", dev_name);
        goto open_failed;
    }

    // 关联链路层
    netif->link_layer = netif_get_link_layer(netif->type);
    if (netif->link_layer == NULL && netif->type != NETIF_TYPE_LOOPBACK)
    {
        dbug_error("netif %s type:%d link layer not registered", dev_name, netif->type);
        goto open_failed;
    }

    // 插入到网卡链表尾部
    nlist_insert_last(&netif_list, &netif->node);
    // display_netif_list();
    return netif;

open_failed:
    if (netif->state == NETIF_STATE_OPENED)
    {
        netif->opts->close(netif);
    }
    fixq_destroy(&netif->in_q);
    fixq_destroy(&netif->out_q);
    mblock_free(&netif_mblock, netif);
    return NULL;
}

net_err_t netif_set_hwaddr(netif_t* netif, const uint8_t* hwaddr, uint8_t hwaddr_len)
{
    if (hwaddr_len > NETIF_HWADDR_LEN)
    {
        return NET_ERR_INVALID_PARAM;
    }
    plat_memcpy(netif->hwaddr.addr, hwaddr, hwaddr_len);
    netif->hwaddr.len = hwaddr_len;
    return NET_ERR_OK;
}

net_err_t netif_set_addr(netif_t* netif, const ipaddr_t* ipaddr, const ipaddr_t* netmask, const ipaddr_t* gateway)
{
    ipaddr_copy(&netif->ipaddr, ipaddr ? ipaddr : get_addr_any());
    ipaddr_copy(&netif->netmask, netmask ? netmask : get_addr_any());
    ipaddr_copy(&netif->gateway, gateway ? gateway : get_addr_any());
    return NET_ERR_OK;
}

net_err_t netif_set_active(netif_t* netif)
{
    if (netif->state != NETIF_STATE_OPENED)
    {
        dbug_error("netif_set_active: invalid state");
        return NET_ERR_INVALID_STATE;
    }
    netif->state = NETIF_STATE_ACTIVE;

    // 设置为默认网卡（如果还没有默认网卡的话）
    if (netif_default == NULL && netif->type != NETIF_TYPE_LOOPBACK)
    {
        netif_default = netif;
    }

    if (netif->link_layer)
    {
        net_err_t err = netif->link_layer->open(netif);
        if (err != NET_ERR_OK)
        {
            dbug_error("netif_set_active: link layer open failed");
            return err;
        }
    }

    display_netif_list();
    return NET_ERR_OK;
}

net_err_t netif_set_inactive(netif_t* netif)
{
    if (netif->state != NETIF_STATE_ACTIVE)
    {
        dbug_error("netif_set_active: invalid state");
        return NET_ERR_INVALID_STATE;
    }
    netif->state = NETIF_STATE_OPENED;

    if (netif->link_layer)
    {
        netif->link_layer->close(netif);
    }

    pktbuf_t* buf;
    // 清空接收队列
    while ((buf = fixq_recv(&netif->in_q, -1)) != NULL)
    {
        pktbuf_free(buf);
    }

    // 清空发送队列
    while ((buf = fixq_recv(&netif->out_q, -1)) != NULL)
    {
        pktbuf_free(buf);
    }

    // 如果是默认网卡，清除默认网卡指针
    if (netif_default == netif)
    {
        netif_default = NULL;
    }
    display_netif_list();
    return NET_ERR_OK;
}

net_err_t netif_close(netif_t* netif)
{
    if (netif->state == NETIF_STATE_ACTIVE)
    {
        dbug_error("netif_close: netif is active");
        return NET_ERR_INVALID_STATE;
    }

    // 关闭网卡接口
    netif->opts->close(netif);

    // 设置状态为关闭
    netif->state = NETIF_STATE_CLOSED;

    // 销毁收发队列
    fixq_destroy(&netif->in_q);
    fixq_destroy(&netif->out_q);

    // 从网卡链表中移除
    nlist_remove(&netif_list, &netif->node);

    // 如果是默认网卡，清除默认网卡指针
    if (netif_default == netif)
    {
        netif_default = NULL;
    }

    display_netif_list();

    mblock_free(&netif_mblock, netif);
    return NET_ERR_OK;
}

net_err_t netif_set_default(netif_t* netif)
{
    netif_default = netif;
    return NET_ERR_OK;
}

net_err_t netif_put_in(netif_t* netif, pktbuf_t* buf, const int tmo)
{
    const net_err_t err = fixq_send(&netif->in_q, buf, tmo);
    if (err != NET_ERR_OK)
    {
        dbug_warn("netif_put_in: send to in_q failed");
        return err;
    }

    exmsg_netif_in(netif);
    return NET_ERR_OK;
}

pktbuf_t* netif_get_in(netif_t* netif, const int tmo)
{
    pktbuf_t* buf = fixq_recv(&netif->in_q, tmo);
    if (buf)
    {
        pktbuf_reset_access(buf);
        return buf;
    }
    dbug_info("netif_get_in: recv from in_q timeout");
    return NULL;
}

net_err_t netif_put_out(netif_t* netif, pktbuf_t* buf, const int tmo)
{
    const net_err_t err = fixq_send(&netif->out_q, buf, tmo);
    if (err != NET_ERR_OK)
    {
        dbug_warn("netif_put_out: send to out_q failed");
        return err;
    }
    return NET_ERR_OK;
}

pktbuf_t* netif_get_out(netif_t* netif, const int tmo)
{
    pktbuf_t* buf = fixq_recv(&netif->out_q, tmo);
    if (buf)
    {
        pktbuf_reset_access(buf);
        return buf;
    }
    dbug_info("netif_get_out: recv from out_q timeout");
    return NULL;
}

net_err_t netif_out(netif_t* netif, ipaddr_t* ipaddr, pktbuf_t* buf)
{
    if (netif->state != NETIF_STATE_ACTIVE)
    {
        dbug_error("netif_out: netif is not active");
        return NET_ERR_INVALID_STATE;
    }

    if (netif->link_layer)
    {
        // 以太网发送数据包
        return ether_raw_out(netif, PROTOCOL_TYPE_ARP, ether_broadcast_addr(), buf);
    }

    const net_err_t err = netif_put_out(netif, buf, -1);
    if (err != NET_ERR_OK)
    {
        dbug_error("netif_out: put out failed");
        return err;
    }
    return netif->opts->linkoutput(netif);
}

net_err_t netif_register_link_layer(const link_layer_t* ll)
{
    if (ll->type < NETIF_TYPE_NONE || ll->type >= NETIF_TYPE_SIZE)
    {
        dbug_error("netif_register_link_layer: invalid link layer type");
        return NET_ERR_INVALID_PARAM;
    }

    if (link_layers[ll->type - 1] != NULL)
    {
        dbug_error("netif_register_link_layer: link layer type already registered");
        return NET_ERR_INVALID_PARAM;
    }

    link_layers[ll->type - 1] = ll;
    return NET_ERR_OK;
}
