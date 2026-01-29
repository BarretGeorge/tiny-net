#include "common.h"
#include "dbug.h"
#include "exmsg.h"
#include "netif.h"
#include "timer.h"
#include "loop.h"
#include "arp.h"
#include "ether.h"
#include "netif_pcap.h"

// 本地ip地址
const char local_ip[] = "192.168.100.108";

// 网关地址
const char gateway_ip[] = "192.168.100.1";

// 分配给虚拟网卡的虚拟地址
const char virtual_ip[] = "192.168.100.95";

// 客户端ip地址
const char friend_ip[] = "192.168.100.104";

// 虚拟网卡操作函数
pcap_data_t netdev_0 = {
    .ipaddr = local_ip,
    .hwaddr = (uint8_t[]){0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},
};

static net_err_t netdev_init(void)
{
    netif_t* netif = netif_open("vnet0", &netdev_ops, &netdev_0);
    if (!netif)
    {
        dbug_error("loop_init: failed to open loopback interface");
        return NET_ERR_SYS;
    }

    ipaddr_t ipaddr;
    ipaddr_t netmask;
    ipaddr_t gateway;

    ipaddr4_form_str(&ipaddr, virtual_ip);
    ipaddr4_form_str(&netmask, "255.255.255.0");
    ipaddr4_form_str(&gateway, gateway_ip);

    netif_set_addr(netif, &ipaddr, &netmask, &gateway);

    netif_set_hwaddr(netif, netdev_0.hwaddr, 6);

    netif_set_active(netif);

    return NET_ERR_OK;
}

net_err_t tiny_net_init(void)
{
    // pktbuf初始化
    pktbuf_init();

    // 消息模块初始化
    exmsg_init();

    // 网络接口初始化
    netif_init();

    // 定时器初始化
    net_timer_init();

    // 回环网卡初始化
    loop_init();

    // ARP模块初始化
    arp_init();

    // 以太网卡初始化
    ether_init();

    // 虚拟网卡初始化
    netdev_init();

    // 工作线程
    exmsg_start();

    return NET_ERR_OK;
}
