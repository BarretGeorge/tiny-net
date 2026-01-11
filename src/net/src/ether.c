#include "ether.h"
#include "netif.h"
#include "tool.h"

// 打开函数指针
static net_err_t ether_open(netif_t* netif)
{
    return NET_ERR_OK;
}

// 关闭函数指针
static void ether_close(netif_t* netif)
{
}

// 输入函数指针
static net_err_t ether_input(netif_t* netif, pktbuf_t* buf)
{
    return NET_ERR_OK;
}

// 输出函数指针
static net_err_t ether_output(netif_t* netif, ipaddr_t* ipaddr, pktbuf_t* buf)
{
    return NET_ERR_OK;
}


net_err_t ether_init()
{
    static const link_layer_t ether_link_layer = {
        .type = NETIF_TYPE_ETHERNET,
        .open = ether_open,
        .close = ether_close,
        .input = ether_input,
        .output = ether_output,
    };

    uint32_t addr = 0x12345678;

    plat_printf("ether_header_t sizeof=%lu \n", sizeof(ether_header_t));
    plat_printf("ether_frame_t sizeof=%lu \n", sizeof(ether_frame_t));

    return netif_register_link_layer(&ether_link_layer);
}
