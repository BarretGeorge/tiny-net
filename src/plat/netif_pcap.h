#ifndef TINY_NET_NETIF_PCAP_H
#define TINY_NET_NETIF_PCAP_H

#include "net_err.h"
#include "netif.h"

typedef struct pcap_data_t
{
    // 设备ip地址
    const char* ipaddr;
    // 设备硬件地址
    const uint8_t* hwaddr;
} pcap_data_t;

net_err_t netif_pcap_open(netif_t* netif, void* data);

extern const netif_open_options_t netdev_ops;

#endif //TINY_NET_NETIF_PCAP_H
