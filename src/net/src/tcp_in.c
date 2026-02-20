#include "tcp_in.h"
#include "dbug.h"
#include "tcp_out.h"
#include "tool.h"
#include "tcp_state.h"

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
    static const tcp_state_proc_t proc_table[] = {
        [TCP_STATE_CLOSE] = tcp_close_in,
        [TCP_STATE_LISTEN] = tcp_listen_in,
        [TCP_STATE_SYN_SENT] = tcp_syn_sent_in,
        [TCP_STATE_SYN_RECEIVED] = tcp_syn_received_in,
        [TCP_STATE_ESTABLISHED] = tcp_established_in,
        [TCP_STATE_FIN_WAIT_1] = tcp_fin_wait_1_in,
        [TCP_STATE_FIN_WAIT_2] = tcp_fin_wait_2_in,
        [TCP_STATE_CLOSE_WAIT] = tcp_close_wait_in,
        [TCP_STATE_CLOSING] = tcp_closing_in,
        [TCP_STATE_LAST_ACK] = tcp_last_ack_in,
        [TCP_STATE_TIME_WAIT] = tcp_time_wait_in,
    };

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

    tcp_t* tcp = tcp_find(dest_ip, header->dest_port, src_ip, header->src_port);
    if (tcp == NULL)
    {
        dbug_warn(DBG_MOD_TCP, "tcp_input: no matching socket for dest port %d", header->dest_port);
        tcp_send_reset(&seg);
        pktbuf_free(buf);
        return NET_ERR_OK;
    }

    net_err_t err = proc_table[tcp->state](tcp, &seg);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "tcp_input: state proc failed, err=%d", err);
        return err;
    }
    return NET_ERR_OK;
}

net_err_t tcp_data_in(tcp_t* tcp, tcp_seg_t* seg)
{
    int wakeup = 0;
    tcp_header_t* header = seg->header;
    if (header->f_fin)
    {
        tcp->recv.next_seq++;
        wakeup++;
    }

    if (wakeup > 0)
    {
        if (header->f_fin)
        {
            sock_wakeup(&tcp->base, SOCK_WAIT_ALL, NET_ERR_CLOSE);
        }
        else
        {
            sock_wakeup(&tcp->base, SOCK_WAIT_READ, NET_ERR_OK);
        }

        tcp_send_ack(tcp, seg);
    }

    return NET_ERR_OK;
}
