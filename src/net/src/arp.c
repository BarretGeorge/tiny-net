#include "arp.h"
#include "dbug.h"
#include "mblock.h"
#include "tool.h"
#include "protocol.h"

static arp_entity_t cache_tbl[ARP_CACHE_SIZE];

static mblock_t cache_block;

static nlist_t cache_list;

static net_err_t cache_init()
{
    nlist_init(&cache_list);

    net_err_t err = mblock_init(&cache_block, &cache_tbl, sizeof(arp_entity_t), ARP_CACHE_SIZE, NLOCKER_TYPE_NONE);
    if (err != NET_ERR_OK)
    {
        return err;
    }
    return NET_ERR_OK;
}

net_err_t arp_init()
{
    net_err_t err = cache_init();
    if (err != NET_ERR_OK)
    {
        dbug_error("");
        return err;
    }
    return NET_ERR_OK;
}

net_err_t arp_make_request(netif_t* netif, const ipaddr_t* addr)
{
    pktbuf_t* buf = pktbuf_alloc(sizeof(arp_pkt_t));
    if (buf == NULL)
    {
        dbug_error("pktbuf_alloc fail");
        return NET_ERR_MEM;
    }

    pktbuf_set_cont(buf, sizeof(arp_pkt_t));

    arp_pkt_t* arp_pkt = (arp_pkt_t*)pktbuf_data(buf);
    arp_pkt->h_type = x_htons(ARP_HW_ETHER);
    arp_pkt->p_type = x_htons(PROTOCOL_TYPE_IPv4);
    arp_pkt->hw_len = ETHER_HWADDR_LEN;
    arp_pkt->p_len = IPV4_ADDR_LEN;
    arp_pkt->opcode = x_htons(ARP_REQUEST);
    plat_memcpy(arp_pkt->sender_hwaddr, netif->hwaddr.addr, ETHER_HWADDR_LEN);
    ipaddr_to_buf(&netif->ipaddr, arp_pkt->sender_addr);
    plat_memset(arp_pkt->target_hwaddr, 0, ETHER_HWADDR_LEN);
    ipaddr_to_buf(addr, arp_pkt->target_addr);

    net_err_t err = ether_raw_out(netif, PROTOCOL_TYPE_ARP, ether_broadcast_addr(), buf);
    if (err != NET_ERR_OK)
    {
        dbug_error("ether_raw_out fail");
        pktbuf_free(buf);
        return err;
    }

    return NET_ERR_OK;
}

net_err_t arp_make_gratuitous_request(netif_t* netif)
{
    return arp_make_request(netif, &netif->ipaddr);
}
