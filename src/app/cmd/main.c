#include "arp.h"
#include "net.h"
#include "netif_pcap.h"
#include "dbug.h"
#include "exmsg.h"
#include "loop.h"
#include "sys.h"
#include "timer.h"

// 本地ip地址
const char local_ip[] = "192.168.100.108";

// 网关地址
const char gateway_ip[] = "192.168.100.1";

// 分配给虚拟网卡的虚拟地址
const char virtual_ip[] = "192.168.100.95";

// 客户端ip地址
const char friend_ip[] = "192.168.100.104";

// 广播地址
const char broadcast_ip[] = "255.255.255.255";

// 区域广播地址
const char directed_broadcast_ip[] = "192.168.100.255";

// 虚拟网卡操作函数
pcap_data_t netdev_0 = {
    .ipaddr = local_ip,
    .hwaddr = (uint8_t[]){0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},
};

void test_ethernet_send(netif_t* netif)
{
    pktbuf_t* buf = pktbuf_alloc(32);
    pktbuf_fill(buf, 0x53, 32);

    ipaddr_t addr;
    ipaddr4_form_str(&addr, friend_ip);
    netif_out(netif, &addr, buf);
    dbug_info("test_ethernet_send: sent 32 bytes to %s", friend_ip);

    /**
     * buf 不需要 free
     *
     * 因为 netif_out 已经将 buf 的所有权转移给了发送队列，由发送队列负责释放
     */

    buf = pktbuf_alloc(64);
    pktbuf_set_cont(buf, 64);
    pktbuf_fill(buf, 0xAA, 64);
    ipaddr4_form_str(&addr, broadcast_ip);
    netif_out(netif, &addr, buf);
    dbug_info("test_ethernet_send: sent 64 bytes to %s", broadcast_ip);
}

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

    // 测试发送以太网帧
    test_ethernet_send(netif);
    return NET_ERR_OK;
}

int main(void)
{
    pktbuf_init();

    exmsg_init();

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

    while (true)
    {
        sys_sleep(100);
    }
}
