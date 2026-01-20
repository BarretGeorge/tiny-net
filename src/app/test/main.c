#include <stdio.h>
#include "sys_plat.h"
#include "net.h"
#include "netif_pcap.h"
#include "dbug.h"
#include "mblock.h"
#include "pktbuf.h"
#include "netif.h"
#include "loop.h"
#include "exmsg.h"
#include "ether.h"
#include "timer.h"

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

net_err_t netdev_init(void)
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

    // todo 测试以太网数据发送
    pktbuf_t* buf = pktbuf_alloc(32);
    pktbuf_fill(buf, 0x53, 32);

    ipaddr_t addr;
    ipaddr4_form_str(&addr, friend_ip);
    netif_out(netif, &addr, buf);
    return NET_ERR_OK;
}

void netif_test()
{
    pktbuf_init();

    exmsg_init();

    netif_init();

    // 定时器初始化
    net_timer_init();

    // todo 定时器加入测试数据
    // timer_test();

    // 回环网卡初始化
    loop_init();

    // 以太网卡初始化
    ether_init();

    // 虚拟网卡初始化
    netdev_init();

    // 工作线程
    exmsg_start();

    while (true)
    {
        sys_sleep(100);
    }
}

int main(void)
{
    // mblock_test();

    // pktbuf_test();

    // timer_test();

    netif_test();

    return 0;
}
