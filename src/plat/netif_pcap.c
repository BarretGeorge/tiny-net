#include "netif_pcap.h"
#include "sys_plat.h"
#include "exmsg.h"
#include "dbug.h"

void recv_thread(void* arg)
{
    plat_printf("pcap recv_thread started\n");
    while (1)
    {
        sys_sleep(200);
        // exmsg_netif_in();
    }
}

void send_thread(void* arg)
{
    plat_printf("pcap send_thread started\n");
    while (1)
    {
        sys_sleep(100);
    }
}

net_err_t netif_pcap_open(netif_t* netif, void* data)
{
    pcap_data_t* pcap_data = (pcap_data_t*)data;
    pcap_t* pcap = pcap_device_open(pcap_data->ipaddr, pcap_data->hwaddr);
    if (!pcap)
    {
        dbug_error("netif_pcap_open: failed to open pcap device");
        return NET_ERR_IO;
    }
    netif->type = NETIF_TYPE_ETHERNET;
    netif->mtu = 1500;

    netif->opts_data = pcap;
    netif_set_hwaddr(netif, pcap_data->hwaddr, 6);

    const sys_thread_t recv_th = sys_thread_create(recv_thread, netif);
    if (recv_th == SYS_THREAD_INVALID)
    {
        return NET_ERR_SYS;
    }
    const sys_thread_t send_th = sys_thread_create(send_thread, netif);
    if (send_th == SYS_THREAD_INVALID)
    {
        return NET_ERR_SYS;
    }
    return NET_ERR_OK;
}

net_err_t netif_pcap_close(netif_t* netif)
{
    pcap_close(netif->opts_data);
    return NET_ERR_OK;
}

net_err_t netif_pcap_output(netif_t* netif)
{
    return NET_ERR_OK;
}

const netif_open_options_t netdev_ops = {
    .open = netif_pcap_open,
    .close = netif_pcap_close,
    .linkoutput = netif_pcap_output,
};
