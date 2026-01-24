#include "ipv4.h"
#include "dbug.h"
#include "tool.h"

static void ipv4_header_ntohs(ipv4_header_t* hdr)
{
    hdr->total_len = x_ntohs(hdr->total_len);
    hdr->id = x_ntohs(hdr->id);
    hdr->frag_all = x_ntohs(hdr->frag_all);
    hdr->header_checksum = x_ntohs(hdr->header_checksum);
}

static net_err_t ipv4_pkt_is_valid(const ipv4_pkt_t* pkt, const uint32_t size, netif_t* netif)
{
    // 检查版本号
    if (pkt->header.version != IPADDR_TYPE_V4)
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

    return NET_ERR_OK;
}

int ipv4_hdr_size(const ipv4_pkt_t* pkt)
{
    return pkt->header.shdr * 4;
}

net_err_t ipv4_init()
{
    dbug_info("ipv4_init");
    return NET_ERR_OK;
}

net_err_t ipv4_input(netif_t* netif, pktbuf_t* buf)
{
    dbug_info("ipv4_input");

    // 设置包的连续性
    pktbuf_set_cont(buf, sizeof(ipv4_header_t));


    ipv4_pkt_t* pkt = (ipv4_pkt_t*)pktbuf_data(buf);
    net_err_t err = ipv4_pkt_is_valid(pkt, buf->total_size, netif);
    if (err != NET_ERR_OK)
    {
        dbug_warn("ipv4_input: invalid ipv4 packet, err=%d", err);
        pktbuf_free(buf);
        return err;
    }

    ipv4_header_ntohs(&pkt->header);

    // 调整包的长度，删除填充部分
    if ((err = pktbuf_resize(buf, ipv4_hdr_size(pkt))) != NET_ERR_OK)
    {
        dbug_warn("ipv4_input: pktbuf_set_cont failed, err=%d", err);
        pktbuf_free(buf);
        return err;
    }

    pktbuf_free(buf);
    return NET_ERR_OK;
}
