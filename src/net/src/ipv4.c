#include "ipv4.h"
#include "dbug.h"
#include "protocol.h"
#include "tool.h"
#include "icmp_v4.h"
#include "mblock.h"

static uint16_t packet_id = 0;

static ip_fragment_t fragments[IPV4_FRAGS_MAX_NR];

static mblock_t fragment_mblock;

static nlist_t fragment_list;

static net_err_t fragment_init()
{
    nlist_init(&fragment_list);
    net_err_t err = mblock_init(&fragment_mblock, fragments, sizeof(ip_fragment_t), IPV4_FRAGS_MAX_NR,
                                NLOCKER_TYPE_NONE);
    if (err != NET_ERR_OK)
    {
        dbug_error("fragment_init: mblock_init failed, err=%d", err);
        return err;
    }
    return NET_ERR_OK;
}

#if DBG_DISPLAY_ENABLE(DBG_IPV4)
static void display_ipv4_header(const ipv4_pkt_t* pkt)
{
    const ipv4_header_t* hdr = &pkt->header;
    plat_printf("IPv4 Header:\n");
    plat_printf("  Version: %u\n", hdr->version);
    plat_printf("  Header Length: %u bytes\n", hdr->shdr * 4);
    plat_printf("  Type of Service: 0x%02X\n", hdr->tos);
    plat_printf("  Total Length: %u bytes\n", x_ntohs(hdr->total_len));
    plat_printf("  Identification: 0x%04X\n", x_ntohs(hdr->id));
    plat_printf("  Flags and Fragment Offset: 0x%04X\n", x_ntohs(hdr->frag_offset));
    plat_printf("  More Fragments: %s\n", hdr->more_frags ? "Yes" : "No");
    plat_printf("  Time to Live: %u\n", hdr->ttl);
    plat_printf("  Protocol: %u\n", hdr->protocol);
    plat_printf("  Header Checksum: 0x%04X\n", x_ntohs(hdr->header_checksum));
    plat_printf("  Source Address: %u.%u.%u.%u\n",
                hdr->src_addr[0], hdr->src_addr[1], hdr->src_addr[2], hdr->src_addr[3]);
    plat_printf("  Destination Address: %u.%u.%u.%u\n",
                hdr->dest_addr[0], hdr->dest_addr[1], hdr->dest_addr[2], hdr->dest_addr[3]);
}
#else
#define  display_ipv4_header(header)
#endif

static void ipv4_header_ntohs(ipv4_header_t* hdr)
{
    hdr->total_len = x_ntohs(hdr->total_len);
    hdr->id = x_ntohs(hdr->id);
    hdr->frag_all = x_ntohs(hdr->frag_all);
    hdr->header_checksum = x_ntohs(hdr->header_checksum);
}

static void ipv4_header_htonl(ipv4_header_t* hdr)
{
    hdr->total_len = x_htons(hdr->total_len);
    hdr->id = x_htons(hdr->id);
    hdr->frag_all = x_htons(hdr->frag_all);
    hdr->header_checksum = x_htons(hdr->header_checksum);
}

static net_err_t ipv4_pkt_is_valid(const ipv4_pkt_t* pkt, const uint32_t size, netif_t* netif)
{
    // 检查版本号
    if (pkt->header.version != NET_VERSION_IPV4)
    {
        dbug_warn("ipv4_pkt_is_valid: invalid version %d", pkt->header.version);
        return NET_ERR_FRAME;
    }

    int hdr_len = ipv4_hdr_size(pkt);
    if (hdr_len < sizeof(ipv4_header_t))
    {
        dbug_warn("ipv4_pkt_is_valid: invalid header length %d", hdr_len);
        return NET_ERR_FRAME;
    }

    int total_len = x_ntohs(pkt->header.total_len);
    if (total_len < hdr_len || total_len > size)
    {
        dbug_warn("ipv4_pkt_is_valid: invalid total length %d", total_len);
        return NET_ERR_FRAME;
    }


    // 检查校验和
    if (pkt->header.header_checksum)
    {
        uint16_t checksum = checksum16(&pkt->header, hdr_len, 0, true);
        if (checksum != 0)
        {
            dbug_warn("ipv4_pkt_is_valid: invalid header checksum 0x%04X", checksum);
            return NET_ERR_CHECKSUM;
        }
    }

    return NET_ERR_OK;
}

static net_err_t ip_normal_input(netif_t* netif, pktbuf_t* buf, const ipaddr_t* src_ip, const ipaddr_t* dest_ip)
{
    ipv4_pkt_t* ipv4_pkt = (ipv4_pkt_t*)pktbuf_data(buf);
    display_ipv4_header(ipv4_pkt);
    net_err_t err = NET_ERR_OK;
    switch (ipv4_pkt->header.protocol)
    {
    case PROTOCOL_TYPE_ICMP_V4: // ICMP
        err = icmp_v4_input(src_ip, &netif->ipaddr, buf);
        break;
    case PROTOCOL_TYPE_UDP: // UDP
        // 测试端口不可达
        ipv4_header_ntohs(&ipv4_pkt->header);
        err = icmp_v4_output_unreach(src_ip, &netif->ipaddr, ICMP_V4_CODE_UNREACH_PORT, buf);
        break;
    case PROTOCOL_TYPE_TCP: // TCP
        break;
    default:
        dbug_warn("ip_normal_input: unsupported protocol %d", ipv4_pkt->header.protocol);
        return NET_ERR_FRAME;
    }

    if (err != NET_ERR_OK)
    {
        dbug_warn("ip_normal_input: protocol %d input failed, err=%d", ipv4_pkt->header.protocol, err);
    }
    return err;
}

int ipv4_hdr_size(const ipv4_pkt_t* pkt)
{
    return pkt->header.shdr * 4;
}

void ipv4_set_hdr_size(ipv4_pkt_t* pkt, const int size)
{
    pkt->header.shdr = size / 4;
}

net_err_t ipv4_init()
{
    dbug_info("ipv4_init");
    net_err_t err = fragment_init();
    if (err != NET_ERR_OK)
    {
        dbug_error("ipv4_init: fragment_init failed, err=%d", err);
        return err;
    }
    return NET_ERR_OK;
}

net_err_t ipv4_input(netif_t* netif, pktbuf_t* buf)
{
    dbug_info("ipv4_input");

    // 设置包的连续性
    pktbuf_set_cont(buf, sizeof(ipv4_header_t));

    ipv4_pkt_t* pkt = (ipv4_pkt_t*)pktbuf_data(buf);

    // 验证ipv4包是否合法
    net_err_t err = ipv4_pkt_is_valid(pkt, buf->total_size, netif);
    if (err != NET_ERR_OK)
    {
        dbug_warn("ipv4_input: invalid ipv4 packet, err=%d", err);
        return err;
    }

    ipv4_header_ntohs(&pkt->header);

    // 调整包的长度 为实际长度
    if ((err = pktbuf_resize(buf, pkt->header.total_len)) < 0)
    {
        dbug_warn("ipv4_input: pktbuf_resize failed, err=%d", err);
        return err;
    }

    ipaddr_t src_ip, dest_ip;
    ipaddr4_form_buf(&src_ip, pkt->header.src_addr);
    ipaddr4_form_buf(&dest_ip, pkt->header.dest_addr);

    if (!ipaddr_is_match(&dest_ip, &netif->ipaddr, &netif->netmask))
    {
        dbug_warn("ipv4_input: packet not for us,dest ip %s", dest_ip.a_addr);
        return NET_ERR_TARGET_ADDR_MATCH;
    }

    err = ip_normal_input(netif, buf, &src_ip, &dest_ip);
    if (err != NET_ERR_OK)
    {
        dbug_warn("ipv4_input: ip_normal_input failed, err=%d", err);
        pktbuf_free(buf);
        return err;
    }

    return NET_ERR_OK;
}

net_err_t ipv4_output(const uint8_t protocol, const ipaddr_t* dest_ip, const ipaddr_t* src_ip, pktbuf_t* buf)
{
    // 添加IPv4头
    net_err_t err = pktbuf_add_header(buf, sizeof(ipv4_header_t), true);
    if (err != NET_ERR_OK)
    {
        dbug_error("ipv4_output: pktbuf_add_header failed, err=%d", err);
        return err;
    }

    ipv4_pkt_t* pkt = (ipv4_pkt_t*)pktbuf_data(buf);
    pkt->header.shar_all = 0;
    pkt->header.version = NET_VERSION_IPV4;
    ipv4_set_hdr_size(pkt, sizeof(ipv4_header_t));
    pkt->header.total_len = (uint16_t)buf->total_size;
    pkt->header.id = ++packet_id;
    pkt->header.frag_all = 0;
    pkt->header.ttl = 64;
    pkt->header.header_checksum = 0;
    pkt->header.protocol = protocol;
    ipaddr_to_buf(src_ip, pkt->header.src_addr);
    ipaddr_to_buf(dest_ip, pkt->header.dest_addr);

    // 转换为网络字节序
    ipv4_header_htonl(&pkt->header);

    // 重置访问位置
    pktbuf_reset_access(buf);

    // 计算校验和
    pkt->header.header_checksum = pktbuf_checksum16(buf, ipv4_hdr_size(pkt), 0, true);

    display_ipv4_header(pkt);

    err = netif_out(netif_get_default(), (ipaddr_t*)dest_ip, buf);
    if (err != NET_ERR_OK)
    {
        dbug_error("ipv4_output: netif_out failed, err=%d", err);
        return err;
    }
    return NET_ERR_OK;
}
