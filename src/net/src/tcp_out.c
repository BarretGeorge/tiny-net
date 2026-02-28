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

static int copy_send_data(const tcp_t* tcp, pktbuf_t* buf, int data_offset, int data_len)
{
    if (data_len == 0)
    {
        return 0;
    }
    net_err_t err = pktbuf_resize(buf, (int)sizeof(tcp_header_t) + data_len);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "copy_send_data: pktbuf_resize failed, err=%d", err);
        return -1;
    }
    tcp_header_t* header = (tcp_header_t*)pktbuf_data(buf);
    pktbuf_reset_access(buf);
    pktbuf_seek(buf, (int)tcp_header_size(header));

    tcp_buf_read_send(&tcp->send.buf, buf, data_offset, data_len);
    return data_len;
}

static void get_send_info(const tcp_t* tcp, int* data_offset, int* data_len)
{
    *data_offset = (int)tcp->send.next_seq - (int)tcp->send.un_ack_seq;
    *data_len = tcp_buf_count(&tcp->send.buf) - *data_offset;

    if (*data_len > tcp->mss)
    {
        *data_len = tcp->mss;
    }
}

net_err_t tcp_send_reset(const tcp_seg_t* seg)
{
    tcp_header_t* in = seg->header;

    pktbuf_t* buf = pktbuf_alloc(sizeof(tcp_header_t));
    if (buf == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_send_reset: pktbuf_alloc failed");
        return NET_ERR_MEM;
    }

    tcp_header_t* out = (tcp_header_t*)pktbuf_data(buf);
    out->src_port = in->dest_port;
    out->dest_port = in->src_port;
    out->seq_num = 0;
    out->ack_num = seg->seq + seg->seq_len;
    out->flags = 0;
    out->f_rst = 1;
    out->window_size = 0;
    out->checksum = 0;
    out->urgent_ptr = 0;
    tcp_set_header_size(out, sizeof(tcp_header_t));

    if (in->f_ack)
    {
        out->seq_num = in->ack_num;
        out->ack_num = 0;
        out->f_ack = 0;
    }
    else
    {
        out->ack_num = seg->seq + seg->seq_len;
        out->f_ack = 1;
    }

    return send_out(out, buf, &seg->remote_ip, &seg->local_ip);
}

static void write_tcp_option(const tcp_t* tcp, pktbuf_t* buf)
{
    // 目前只支持MSS选项
    tcp_option_mss_t mss_option;
    mss_option.kind = TCP_OPTION_MSS;
    mss_option.length = sizeof(tcp_option_mss_t);
    mss_option.mss = x_htons(tcp->mss);

    int options_size = sizeof(mss_option);

    net_err_t err = pktbuf_resize(buf, (int)sizeof(tcp_header_t) + options_size);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "write_tcp_option: pktbuf_resize failed, err=%d", err);
        return;
    }
    pktbuf_seek(buf, sizeof(tcp_header_t));
    err = pktbuf_write(buf, (const uint8_t*)&mss_option, sizeof(mss_option));
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "write_tcp_option: pktbuf_write failed, err=%d", err);
        return;
    }
}

net_err_t tcp_transmit(tcp_t* tcp)
{
    int data_len, data_offset;
    get_send_info(tcp, &data_offset, &data_len);
    if (data_len < 0)
    {
        return NET_ERR_OK;
    }

    if (data_len == 0 && tcp->flags.syn_out == 0 && tcp->flags.fin_out == 0)
    {
        return NET_ERR_OK; // 没有数据需要发送
    }

    pktbuf_t* buf = pktbuf_alloc(sizeof(tcp_header_t));
    if (buf == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_transmit: pktbuf_alloc failed");
        return NET_ERR_MEM;
    }
    tcp_header_t* header = (tcp_header_t*)pktbuf_data(buf);
    plat_memset(header, 0, sizeof(tcp_header_t));
    header->src_port = tcp->base.local_port;
    header->dest_port = tcp->base.remote_port;
    header->seq_num = tcp->send.next_seq;
    header->ack_num = tcp->recv.next_seq;
    header->flags = 0;
    header->f_syn = tcp->flags.syn_out;
    header->f_ack = tcp->flags.irs_valid;
    if (tcp_buf_count(&tcp->send.buf) == 0)
    {
        header->f_fin = tcp->flags.fin_out;
    }
    else
    {
        header->f_fin = 0;
    }
    header->window_size = tcp_recv_window_size(tcp);
    header->urgent_ptr = 0;

    if (header->f_syn == 1)
    {
        // 设置options
        write_tcp_option(tcp, buf);
    }

    tcp_set_header_size(header, buf->total_size);

    copy_send_data(tcp, buf, data_offset, data_len);

    tcp->send.next_seq += header->f_syn + header->f_fin + data_len;
    return send_out(header, buf, &tcp->base.remote_ip, &tcp->base.local_ip);
}

net_err_t tcp_send_syn(tcp_t* tcp)
{
    tcp->flags.syn_out = 1;
    tcp_transmit(tcp);
    return NET_ERR_OK;
}

net_err_t tcp_ack_process(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;

    // 如果之前发送了SYN报文，并且收到了对方的ACK报文，说明SYN报文已经被确认了，可以将未确认的序列号加1，并清除SYN标志
    if (tcp->flags.syn_out)
    {
        tcp->send.un_ack_seq++;
        tcp->flags.syn_out = 0;
    }

    int ack_count = (int)header->ack_num - (int)tcp->send.un_ack_seq;
    int un_ack_count = (int)tcp->send.next_seq - (int)tcp->send.un_ack_seq;
    int clamped_ack_count = ack_count < un_ack_count ? ack_count : un_ack_count;
    if (clamped_ack_count > 0)
    {
        sock_wakeup(&tcp->base, SOCK_WAIT_WRITE, NET_ERR_OK);
        tcp->send.un_ack_seq += clamped_ack_count;

        clamped_ack_count -= tcp_buf_remove(&tcp->send.buf, clamped_ack_count);
        if (tcp->flags.fin_out && clamped_ack_count > 0)
        {
            tcp->flags.fin_out = 0;
        }
    }
    return NET_ERR_OK;
}

net_err_t tcp_send_ack(tcp_t* tcp, tcp_seg_t* seg)
{
    pktbuf_t* buf = pktbuf_alloc(sizeof(tcp_header_t));
    if (buf == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_send_ack: pktbuf_alloc failed");
        return NET_ERR_MEM;
    }
    tcp_header_t* header = (tcp_header_t*)pktbuf_data(buf);

    plat_memset(header, 0, sizeof(tcp_header_t));
    header->src_port = tcp->base.local_port;
    header->dest_port = tcp->base.remote_port;
    header->seq_num = tcp->send.next_seq;
    header->ack_num = tcp->recv.next_seq;
    header->flags = 0;
    tcp_set_header_size(header, sizeof(tcp_header_t));
    header->f_syn = 0;
    header->f_ack = 1;
    header->window_size = tcp_recv_window_size(tcp);
    header->urgent_ptr = 0;

    return send_out(header, buf, &tcp->base.remote_ip, &tcp->base.local_ip);
}

net_err_t tcp_send_fin(tcp_t* tcp)
{
    tcp->flags.fin_out = 1;
    tcp_transmit(tcp);
    return NET_ERR_OK;
}

int tcp_write_send_buf(tcp_t* tcp, const uint8_t* buf, const size_t len)
{
    int available = tcp_buf_available(&tcp->send.buf);
    if (available <= 0)
    {
        return 0;
    }
    int write_len = len < available ? (int)len : available;
    tcp_buf_write_send(&tcp->send.buf, buf, write_len);
    return write_len;
}
