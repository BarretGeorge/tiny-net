#include "tcp_in.h"
#include "dbug.h"
#include "tool.h"

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

    return NET_ERR_OK;
}
