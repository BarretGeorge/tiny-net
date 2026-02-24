#include "tcp_state.h"
#include "dbug.h"
#include "tcp_out.h"
#include "tcp_in.h"

const char* tcp_state_name(const tcp_state_t state)
{
    static const char* state_names[TCP_STATE_MAX] = {
        "FREE",
        "CLOSE",
        "LISTEN",
        "SYN_SENT",
        "SYN_RECEIVED",
        "ESTABLISHED",
        "FIN_WAIT_1",
        "FIN_WAIT_2",
        "CLOSE_WAIT",
        "CLOSING",
        "LAST_ACK",
        "TIME_WAIT"
    };
    return state < TCP_STATE_MAX ? state_names[state] : "UNKNOWN";
}

void tcp_set_state(tcp_t* tcp, const tcp_state_t state)
{
    dbug_info(DBG_MOD_TCP, "TCP state change: %s -> %s",
              tcp_state_name(tcp->state), tcp_state_name(state));
    tcp->state = state;
}

net_err_t tcp_close_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;
    if (header->f_rst == 0)
    {
        tcp_send_reset(seg);
    }
    return NET_ERR_OK;
}

net_err_t tcp_listen_in(tcp_t* tcp, tcp_seg_t* seg)
{
    return NET_ERR_OK;
}

net_err_t tcp_syn_sent_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;
    if (header->f_ack) // 如果是ACK报文，检查ack_num是否合法
    {
        if (header->ack_num - tcp->send.isn <= 0 || header->ack_num - tcp->send.next_seq > 0)
        {
            dbug_warn(DBG_MOD_TCP, "Received ACK with invalid ack_num: %u", header->ack_num);
            return tcp_send_reset(seg);
        }
    }

    if (header->f_rst) // 如果是RST报文，关闭连接
    {
        if (!header->f_ack)
        {
            return NET_ERR_OK;
        }
        return tcp_abort(tcp, NET_ERR_REST);
    }

    if (header->f_syn) // 如果是SYN报文，进入SYN_RECEIVED状态并回复ACK+SYN
    {
        tcp->recv.isn = header->seq_num;
        tcp->recv.next_seq = header->seq_num + 1;
        tcp->flags.irs_valid = 1;

        tcp_read_options(tcp, header);

        if (header->ack_num)
        {
            tcp_ack_process(tcp, seg);
        }

        if (header->ack_num)
        {
            tcp_send_ack(tcp, seg);
            tcp_set_state(tcp, TCP_STATE_ESTABLISHED);
            sock_wakeup(&tcp->base, SOCK_WAIT_CONN, NET_ERR_OK);
        }
        else
        {
            tcp_set_state(tcp, TCP_STATE_SYN_RECEIVED);
            tcp_send_syn(tcp);
        }
    }

    return NET_ERR_OK;
}

net_err_t tcp_syn_received_in(tcp_t* tcp, tcp_seg_t* seg)
{
    return NET_ERR_OK;
}

net_err_t tcp_established_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;

    if (header->f_rst) // 是否是rest报文
    {
        dbug_error(DBG_MOD_TCP, "Received RST in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    if (header->f_syn) // 重复收到syn报文
    {
        dbug_error(DBG_MOD_TCP, "Received duplicate SYN in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    net_err_t err = NET_ERR_OK;
    if ((err = tcp_ack_process(tcp, seg)) != NET_ERR_OK) // 处理ACK报文
    {
        dbug_warn(DBG_MOD_TCP, "ACK processing failed in ESTABLISHED state");
        return err;
    }

    tcp_data_in(tcp, seg);

    // 是否是关闭连接的请求
    if (header->f_fin)
    {
        tcp_set_state(tcp, TCP_STATE_CLOSE_WAIT);
    }
    return err;
}

void tcp_time_wait(tcp_t* tcp)
{
    tcp_set_state(tcp, TCP_STATE_TIME_WAIT);
}

net_err_t tcp_fin_wait_1_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;

    if (header->f_rst) // 是否是rest报文
    {
        dbug_error(DBG_MOD_TCP, "Received RST in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    if (header->f_syn) // 重复收到syn报文
    {
        dbug_error(DBG_MOD_TCP, "Received duplicate SYN in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    net_err_t err = NET_ERR_OK;
    if ((err = tcp_ack_process(tcp, seg)) != NET_ERR_OK) // 处理ACK报文
    {
        dbug_warn(DBG_MOD_TCP, "ACK processing failed in ESTABLISHED state");
        return err;
    }

    tcp_data_in(tcp, seg);

    if (tcp->flags.fin_out == 0)
    {
        if (header->f_fin) // 是否是关闭连接的请求
        {
            tcp_time_wait(tcp);
        }
        else
        {
            tcp_set_state(tcp, TCP_STATE_FIN_WAIT_2);
        }
    }
    else
    {
        tcp_set_state(tcp, TCP_STATE_CLOSING);
    }

    return NET_ERR_OK;
}

net_err_t tcp_fin_wait_2_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;

    if (header->f_rst) // 是否是rest报文
    {
        dbug_error(DBG_MOD_TCP, "Received RST in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    if (header->f_syn) // 重复收到syn报文
    {
        dbug_error(DBG_MOD_TCP, "Received duplicate SYN in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    net_err_t err = NET_ERR_OK;
    if ((err = tcp_ack_process(tcp, seg)) != NET_ERR_OK) // 处理ACK报文
    {
        dbug_warn(DBG_MOD_TCP, "ACK processing failed in ESTABLISHED state");
        return err;
    }

    tcp_data_in(tcp, seg);

    // 是否是关闭连接的请求
    if (header->f_fin)
    {
        tcp_send_ack(tcp, seg);
        tcp_time_wait(tcp);
    }
    return NET_ERR_OK;
}

net_err_t tcp_close_wait_in(tcp_t* tcp, tcp_seg_t* seg)
{
    return NET_ERR_OK;
}

net_err_t tcp_closing_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;
    if (header->f_rst) // 是否是rest报文
    {
        dbug_error(DBG_MOD_TCP, "Received RST in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    if (header->f_syn) // 重复收到syn报文
    {
        dbug_error(DBG_MOD_TCP, "Received duplicate SYN in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    net_err_t err = NET_ERR_OK;
    if ((err = tcp_ack_process(tcp, seg)) != NET_ERR_OK) // 处理ACK报文
    {
        dbug_warn(DBG_MOD_TCP, "ACK processing failed in ESTABLISHED state");
        return err;
    }

    if (tcp->flags.fin_out == 0)
    {
        tcp_time_wait(tcp);
    }

    return NET_ERR_OK;
}

net_err_t tcp_last_ack_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;
    if (header->f_rst) // 是否是rest报文
    {
        dbug_error(DBG_MOD_TCP, "Received RST in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    if (header->f_syn) // 重复收到syn报文
    {
        dbug_error(DBG_MOD_TCP, "Received duplicate SYN in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    net_err_t err = NET_ERR_OK;
    if ((err = tcp_ack_process(tcp, seg)) != NET_ERR_OK) // 处理ACK报文
    {
        dbug_warn(DBG_MOD_TCP, "ACK processing failed in ESTABLISHED state");
        return err;
    }

    return tcp_abort(tcp, NET_ERR_CLOSE);
}

net_err_t tcp_time_wait_in(tcp_t* tcp, tcp_seg_t* seg)
{
    tcp_header_t* header = seg->header;
    if (header->f_rst) // 是否是rest报文
    {
        dbug_error(DBG_MOD_TCP, "Received RST in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    if (header->f_syn) // 重复收到syn报文
    {
        dbug_error(DBG_MOD_TCP, "Received duplicate SYN in ESTABLISHED state");
        return tcp_abort(tcp, NET_ERR_REST);
    }

    net_err_t err = NET_ERR_OK;
    if ((err = tcp_ack_process(tcp, seg)) != NET_ERR_OK) // 处理ACK报文
    {
        dbug_warn(DBG_MOD_TCP, "ACK processing failed in ESTABLISHED state");
        return err;
    }

    return NET_ERR_OK;
}
