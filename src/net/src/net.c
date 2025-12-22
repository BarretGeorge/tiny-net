#include "net.h"
#include "exmsg.h"
#include "net_plat.h"
#include "pktbuf.h"

net_err_t net_init()
{
    // 平台相关初始化
    net_plat_init();

    // 消息处理模块初始化
    exmsg_init();

    // 数据包缓冲区初始化
    pktbuf_init();
    return NET_ERR_OK;
}

net_err_t net_start()
{
    exmsg_start();
    return NET_ERR_OK;
}
