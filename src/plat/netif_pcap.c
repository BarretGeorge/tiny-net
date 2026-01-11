#include "netif_pcap.h"
#include "sys_plat.h"
#include "exmsg.h"
#include "dbug.h"

static bool is_running = true;

void stop_pcap_netif()
{
    is_running = false;
}

void recv_thread(void* arg)
{
    plat_printf("pcap recv_thread started\n");
    netif_t* netif = (netif_t*)arg;
    pcap_t* pcap = (pcap_t*)netif->opts_data;

    while (is_running)
    {
        // 从pcap读取数据包
        struct pcap_pkthdr* header;
        const u_char* data;
        if (pcap_next_ex(pcap, &header, &data) != 1)
        {
            continue;
        }
        // 分配pktbuf并写入数据
        pktbuf_t* buf = pktbuf_alloc((int)header->len);
        if (!buf)
        {
            dbug_warn("pcap recv_thread: pktbuf_alloc failed");
            continue;
        }
        pktbuf_write(buf, data, (int)header->len);

        // 发送到输入队列
        net_err_t err = netif_put_in(netif, buf, 0);
        if (err != NET_ERR_OK)
        {
            dbug_warn("pcap recv_thread: netif_put_in failed");
            pktbuf_free(buf);
            continue;
        }
    }
}

void send_thread(void* arg)
{
    plat_printf("pcap send_thread started\n");
    netif_t* netif = (netif_t*)arg;
    pcap_t* pcap = (pcap_t*)netif->opts_data;


    // 以太网帧最大长度
    // MTU(1500) + 目的MAC(6) + 源MAC(6) + 类型(2) = 1514字节 (不含FCS)
    static uint8_t rw_buffer[1500 + 6 + 6 + 2];
    while (is_running)
    {
        pktbuf_t* buf = netif_get_out(netif, 0);
        if (buf == NULL)
        {
            dbug_error("pcap send_thread: netif_get_out timeout");
            continue;
        }

        int total_size = (int)buf->total_size;
        plat_memset(rw_buffer, 0, sizeof(rw_buffer));
        pktbuf_read(buf, rw_buffer, total_size);
        pktbuf_free(buf);
        if (pcap_inject(pcap, rw_buffer, total_size) == -1)
        {
            dbug_error("pcap send_thread: pcap_inject failed, err:%s", pcap_geterr(pcap));
            continue;
        }
        else
        {
            dbug_info("发送一个数据包完成 size=%d", total_size);
        }
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
