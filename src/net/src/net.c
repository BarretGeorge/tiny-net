#include "net.h"
#include "exmsg.h"
#include "net_plat.h"
#include "pktbuf.h"
#include "dbug.h"
#include "ether.h"
#include "netif.h"
#include "loop.h"
#include "timer.h"
#include "arp.h"

net_err_t net_init()
{
    dbug_info("init net");
    // 平台相关初始化
    net_plat_init();

    // 消息处理模块初始化
    exmsg_init();

    // 数据包缓冲区初始化
    pktbuf_init();

    // 定时器模块初始化
    net_timer_init();

    // 网卡接口初始化
    netif_init();

    // 回环接口初始化
    loop_init();

    // 以太网模块初始化
    ether_init();

    // ARP模块初始化
    arp_init();
    return NET_ERR_OK;
}

net_err_t net_start()
{
    exmsg_start();
    dbug_info("net is started");
    return NET_ERR_OK;
}
