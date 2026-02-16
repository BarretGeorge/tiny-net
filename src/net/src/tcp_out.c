#include "tcp_out.h"
#include "dbug.h"
#include "ipv4.h"
#include "tool.h"
#include "protocol.h"

static net_err_t send_out(tcp_header_t* out, pktbuf_t* buf, const ipaddr_t* remote_ip, const ipaddr_t* local_ip)
{
    out->src_port = x_htons(out->src_port);
    out->dest_port = x_htons(out->dest_port);
    out->seq_num = x_htonl(out->seq_num);
    out->ack_num = x_htonl(out->ack_num);
    out->window_size = x_htons(out->window_size);
    out->urgent_ptr = x_htons(out->urgent_ptr);
    out->checksum = 0;
    out->checksum = checksum16_pseudo(buf, local_ip, remote_ip, PROTOCOL_TYPE_TCP);

    net_err_t err = ipv4_output(PROTOCOL_TYPE_TCP, remote_ip, local_ip, buf);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "send_out: ipv4_output failed, err=%d", err);
        pktbuf_free(buf);
        return err;
    }
    return NET_ERR_OK;
}

net_err_t tcp_send_reset(const tcp_seg_t* seg)
{
    pktbuf_t* buf = pktbuf_alloc(sizeof(tcp_header_t));
    if (buf == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_send_reset: pktbuf_alloc failed");
        return NET_ERR_MEM;
    }

    tcp_header_t* out = (tcp_header_t*)pktbuf_data(buf);
    out->src_port = x_htons(seg->header->dest_port);
    out->dest_port = x_htons(seg->header->src_port);
    out->seq_num = 0;
    out->ack_num = x_htonl(seg->seq + seg->seq_len);
    out->data_offset = sizeof(tcp_header_t) / 4;
    out->flags = 0;
    out->rst = 1;
    out->window_size = 0;
    out->checksum = 0;
    out->urgent_ptr = 0;
    tcp_set_header_size(out, sizeof(tcp_header_t));


    return send_out(out, buf, &seg->remote_ip, &seg->local_ip);
}
