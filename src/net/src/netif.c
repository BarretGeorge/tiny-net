#include "netif.h"
#include "mblock.h"
#include "dbug.h"

// 网卡接口内存块
static netif_t netif_buffer[NETIF_DEV_CNT];

// 网卡接口内存块管理结构体
static mblock_t netif_mblock;

// 网卡接口链表
static nlist_t netif_list;

// 默认网卡接口
static netif_t* netif_default;

net_err_t netif_init()
{
    dbug_info("init netif");

    netif_default = NULL;

    // 初始化网卡链表
    nlist_init(&netif_list);

    // 初始化网卡接口内存块
    mblock_init(&netif_mblock, netif_buffer, sizeof(netif_t), NETIF_DEV_CNT, NLOCKER_TYPE_NONE);
    return NET_ERR_OK;
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

    err = opts->open(netif, opts_data);
    if (err != NET_ERR_OK)
    {
        dbug_error("open netif %s failed", dev_name);
        goto open_failed;
    }
    netif->state = NETIF_STATE_OPENED;
    netif->opts = (netif_open_options_t*)opts;
    netif->opts_data = opts_data;

    if (netif->type == NETIF_TYPE_NONE)
    {
        dbug_error("netif %s type invalid after open", dev_name);
        goto open_failed;
    }

    // 插入到网卡链表尾部
    nlist_insert_last(&netif_list, &netif->node);
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
