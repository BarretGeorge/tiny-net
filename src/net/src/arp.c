#include "arp.h"
#include "dbug.h"
#include "mblock.h"
#include "tool.h"
#include "protocol.h"

static arp_entity_t cache_tbl[ARP_CACHE_SIZE];

static mblock_t cache_block;

static nlist_t cache_list;

#if DBG_DISPLAY_ENABLE(DBG_ARP)
static void display_arp_entity(const arp_entity_t* entity)
{
    plat_printf("ARP Entity:\n");
    plat_printf("  IP:");
    dbug_dump_ipaddr((ipaddr_t*)&entity->p_addr);
    plat_printf("  MAC:");
    dbug_dump_hwaddr(entity->hwaddr, ETHER_HWADDR_LEN);
    plat_printf("  Netif:%s\n", entity->netif ? entity->netif->name : "NULL");
    plat_printf("  Timeout:%d ms\n", entity->timeout);
    plat_printf("  Retry Count:%d\n", entity->retry_cnt);
    plat_printf("  State:%s\n", entity->state == NET_ARP_RESOLVE ? "stable" : "pending");
    plat_printf("  buf:%d\n", nlist_count(&entity->buf_list));
}

static void display_arp_tbl()
{
    plat_printf("ARP table:\n");
    arp_entity_t* entity = cache_tbl;
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
    {
        if (entity->state != NET_ARP_FREE)
        {
            display_arp_entity(entity);
        }
        entity++;
    }
}

static void display_arp_pkt(const arp_pkt_t* pkt)
{
    plat_printf("ARP Packet:\n");
    plat_printf("  Hardware Type: %u\n", x_ntohs(pkt->h_type));
    plat_printf("  Protocol Type: 0x%04X\n", x_ntohs(pkt->p_type));
    plat_printf("  Hardware Address Length: %u\n", pkt->hw_len);
    plat_printf("  Protocol Address Length: %u\n", pkt->p_len);
    plat_printf("  Opcode: %u\n", x_ntohs(pkt->opcode));
    plat_printf("  Sender MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                pkt->sender_hwaddr[0], pkt->sender_hwaddr[1], pkt->sender_hwaddr[2],
                pkt->sender_hwaddr[3], pkt->sender_hwaddr[4], pkt->sender_hwaddr[5]);
    plat_printf("  Sender IP Address: %d.%d.%d.%d\n",
                pkt->sender_addr[0], pkt->sender_addr[1],
                pkt->sender_addr[2], pkt->sender_addr[3]);
    plat_printf("  Target MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                pkt->target_hwaddr[0], pkt->target_hwaddr[1], pkt->target_hwaddr[2],
                pkt->target_hwaddr[3], pkt->target_hwaddr[4], pkt->target_hwaddr[5]);
    plat_printf("  Target IP Address: %d.%d.%d.%d\n",
                pkt->target_addr[0], pkt->target_addr[1],
                pkt->target_addr[2], pkt->target_addr[3]);
}
#else
#define  display_arp_entity(entity)
#define  display_arp_tbl()
#define  display_arp_pkt(pkt)
#endif

// 释放缓存项中的等待发送的包
static void cache_cleanup(arp_entity_t* entity)
{
    // 释放等待发送的包
    nlist_node_t* node;
    while ((node = nlist_remove_first(&entity->buf_list)) != NULL)
    {
        pktbuf_t* buf = nlist_entry(node, pktbuf_t, node);
        pktbuf_free(buf);
    }
}

// 分配一个ARP缓存项，force为1时，强制分配（释放最旧的缓存项）
static arp_entity_t* cache_alloc(const int force)
{
    arp_entity_t* entity = (arp_entity_t*)mblock_alloc(&cache_block, -1);
    if (entity == NULL && force)
    {
        // 强制分配时，释放掉最旧的缓存项
        nlist_node_t* node = nlist_remove_last(&cache_list);
        if (node == NULL)
        {
            dbug_error("cache_alloc: no free arp entity");
            return NULL;
        }
        entity = nlist_entry(node, arp_entity_t, node);
        cache_cleanup(entity);
    }

    if (entity != NULL)
    {
        // 重置缓存项
        plat_memset(entity, 0, sizeof(arp_entity_t));
        entity->state = NET_ARP_FREE;

        // 初始化链表结点
        nlist_node_init(&entity->node);
        nlist_init(&entity->buf_list);
    }
    return entity;
}

// 释放ARP缓存项
static void cache_free(arp_entity_t* entity)
{
    if (entity == NULL)
    {
        return;
    }
    cache_cleanup(entity);
    nlist_remove(&cache_list, &entity->node);
    mblock_free(&cache_block, entity);
}

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

    display_arp_pkt(arp_pkt);

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

net_err_t arp_pkt_is_valid(const netif_t* netif, const arp_pkt_t* arp_pkt, const int size)
{
    // 包大小检查
    if (size < (int)sizeof(arp_pkt_t))
    {
        dbug_warn("arp_pkt_is_valid: invalid arp packet size");
        return NET_ERR_FRAME;
    }
    // 硬件类型、协议类型、地址长度检查
    if (x_ntohs(arp_pkt->h_type) != ARP_HW_ETHER ||
        x_ntohs(arp_pkt->p_type) != PROTOCOL_TYPE_IPv4 ||
        arp_pkt->hw_len != ETHER_HWADDR_LEN ||
        arp_pkt->p_len != IPV4_ADDR_LEN)
    {
        dbug_warn("arp_pkt_is_valid: invalid arp packet fields");
        return NET_ERR_FRAME;
    }

    uint16_t opcode = x_ntohs(arp_pkt->opcode);
    // 操作码检查
    if (opcode != ARP_REQUEST && opcode != ARP_REPLY)
    {
        dbug_warn("arp_pkt_is_valid: invalid arp opcode");
        return NET_ERR_FRAME;
    }

    // 目标IP地址检查，必须是发给本机的ARP请求或应答
    if (plat_memcmp(arp_pkt->target_addr, netif->ipaddr.a_addr, IPV4_ADDR_LEN) != 0)
    {
        dbug_warn("ARP 目标地址不匹配");
        return NET_ERR_TARGET_ADDR_MATCH;
    }
    return NET_ERR_OK;
}

net_err_t arp_in(netif_t* netif, pktbuf_t* buf)
{
    net_err_t err = NET_ERR_OK;
    // 确保包是连续的
    if ((err = pktbuf_set_cont(buf, sizeof(arp_pkt_t)) != NET_ERR_OK))
    {
        dbug_error("arp_in: pktbuf_set_cont failed, err=%d", err);
        return err;
    }

    arp_pkt_t* arp_pkt = (arp_pkt_t*)pktbuf_data(buf);
    err = arp_pkt_is_valid(netif, arp_pkt, pktbuf_total(buf));
    if (err == NET_ERR_TARGET_ADDR_MATCH) // 目标地址不匹配，直接丢弃
    {
        pktbuf_free(buf);
        return NET_ERR_OK;
    }
    if (err != NET_ERR_OK)
    {
        dbug_error("arp_in: invalid arp packet, err=%d", err);
        return err;
    }

    uint16_t opcode = x_ntohs(arp_pkt->opcode);
    // 操作码检查
    switch (opcode)
    {
    case ARP_REQUEST:
        return arp_make_reply(netif, buf);
    case ARP_REPLY:
        break;
    default:
        dbug_warn("arp_in: unknown arp opcode %d", opcode);
        return NET_ERR_FRAME;
    }

    pktbuf_free(buf);
    return NET_ERR_OK;
}

net_err_t arp_make_reply(netif_t* netif, pktbuf_t* buf)
{
    arp_pkt_t* arp_pkt = (arp_pkt_t*)pktbuf_data(buf);
    arp_pkt->opcode = x_htons(ARP_REPLY);
    // 接收方MAC地址变为发送方MAC地址
    plat_memcpy(arp_pkt->target_hwaddr, arp_pkt->sender_hwaddr, ETHER_HWADDR_LEN);
    // 发送方MAC地址变为本机MAC地址
    plat_memcpy(arp_pkt->sender_hwaddr, netif->hwaddr.addr, ETHER_HWADDR_LEN);
    // 交换IP地址
    plat_memcpy(arp_pkt->target_addr, arp_pkt->sender_addr, IPV4_ADDR_LEN);
    // 本机IP地址变为发送方IP地址
    ipaddr_to_buf(&netif->ipaddr, arp_pkt->sender_addr);

    display_arp_pkt(arp_pkt);

    return ether_raw_out(netif, PROTOCOL_TYPE_ARP, arp_pkt->target_hwaddr, buf);
}
