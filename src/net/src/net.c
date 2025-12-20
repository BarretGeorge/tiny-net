#include "net.h"
#include "exmsg.h"
#include "net_plat.h"

net_err_t net_init()
{
    net_plat_init();
    exmsg_init();
    return NET_ERR_OK;
}

net_err_t net_start()
{
    exmsg_start();
    return NET_ERR_OK;
}
