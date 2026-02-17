#include "tcp_in.h"
#include "dbug.h"
#include "tcp_out.h"
#include "tool.h"

static void tcp_seg_init(tcp_seg_t* seg, const ipaddr_t* remote_ip, const ipaddr_t* local_ip, pktbuf_t* buf)
{
    ipaddr_copy(&seg->local_ip, local_ip);
    ipaddr_copy(&seg->remote_ip, remote_ip);
    seg->header = (tcp_header_t*)pktbuf_data(buf);
    seg->buf = buf;
    seg->data_len = buf->total_size - tcp_header_size(seg->header);
    seg->seq = seg->header->seq_num;
    seg->seq_len = seg->data_len + seg->header->f_syn + seg->header->f_fin;
}

net_err_t tcp_input(pktbuf_t* buf, const ipaddr_t* src_ip, const ipaddr_t* dest_ip)
{
    tcp_header_t* header = (tcp_header_t*)pktbuf_data(buf);
    if (header->checksum != 0)
    {
        pktbuf_reset_access(buf);
        uint16_t checksum = checksum16_pseudo(buf, src_ip, dest_ip, IPPROTO_TCP);
        if (checksum != 0)
        {
            dbug_warn(DBG_MOD_TCP, "tcp_input: invalid checksum");
            return NET_ERR_CHECKSUM;
        }
    }
    if (buf->total_size < sizeof(tcp_header_t) || tcp_header_size(header) > (int)buf->total_size)
    {
        dbug_warn(DBG_MOD_TCP, "tcp_input: packet too short");
        return NET_ERR_FRAME;
    }

    if (header->dest_port == 0 || header->src_port == 0)
    {
        dbug_warn(DBG_MOD_TCP, "tcp_input: invalid port");
        return NET_ERR_PORT_UNREACH;
    }

    if (header->flags == 0)
    {
        dbug_warn(DBG_MOD_TCP, "tcp_input: no flags set");
        return NET_ERR_PROTOCOL;
    }

    header->dest_port = x_ntohs(header->dest_port);
    header->src_port = x_ntohs(header->src_port);
    header->seq_num = x_ntohl(header->seq_num);
    header->ack_num = x_ntohl(header->ack_num);
    header->window_size = x_ntohs(header->window_size);
    header->urgent_ptr = x_ntohs(header->urgent_ptr);

    tcp_seg_t seg;
    tcp_seg_init(&seg, src_ip, dest_ip, buf);


    tcp_send_reset(&seg);
    return NET_ERR_OK;
}
