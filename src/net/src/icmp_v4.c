#include "icmp_v4.h"

#include "dbug.h"
#include "ipv4.h"

static net_err_t is_icmp_v4_pkt_valid(const icmp_v4_pkt_t* pkt, const uint32_t size, pktbuf_t* buf)
{
    // 基本长度检查
    if (size <= sizeof(icmp_v4_header_t))
    {
        dbug_warn("is_icmp_v4_pkt_valid: packet size too small %d", size);
        return NET_ERR_FRAME;
    }

    // 计算校验和
    pktbuf_reset_access(buf);
    uint16_t checksum = pktbuf_checksum16(buf, (int)size, 0, true);
    if (checksum != 0)
    {
        dbug_warn("is_icmp_v4_pkt_valid: invalid checksum 0x%04X", checksum);
        return NET_ERR_CHECKSUM;
    }

    return NET_ERR_OK;
}

net_err_t icmp_v4_init(void)
{
    dbug_info(" ICMPv4 module init");

    return NET_ERR_OK;
}

net_err_t icmp_v4_input(const ipaddr_t* src_ip, const ipaddr_t* netif_ip, pktbuf_t* buf)
{
    ipv4_pkt_t* ip_pkt = (ipv4_pkt_t*)pktbuf_data(buf);

    int ip_hdr_size = ipv4_hdr_size(ip_pkt);
    net_err_t err = pktbuf_set_cont(buf, ip_hdr_size + (int)sizeof(icmp_v4_header_t));
    if (err != NET_ERR_OK)
    {
        dbug_error("icmp_v4_input: pktbuf_set_cont failed, err=%d", err);
        return err;
    }

    // 重新获取 ip_pkt 指针
    ip_pkt = (ipv4_pkt_t*)pktbuf_data(buf);

    if ((err = pktbuf_remove_header(buf, ip_hdr_size)) != NET_ERR_OK)
    {
        dbug_error("icmp_v4_input: pktbuf_remove_header failed, err=%d", err);
        return err;
    }

    icmp_v4_pkt_t* icmp_pkt = (icmp_v4_pkt_t*)pktbuf_data(buf);
    if ((err = is_icmp_v4_pkt_valid(icmp_pkt, buf->total_size, buf)) != NET_ERR_OK)
    {
        dbug_warn("icmp_v4_input: invalid icmpv4 packet");
        return NET_ERR_FRAME;
    }
    return NET_ERR_OK;
}
