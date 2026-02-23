#include "tcp.h"
#include "dbug.h"
#include "mblock.h"
#include "socket.h"
#include "tool.h"
#include "random.h"
#include "tcp_out.h"
#include "tcp_state.h"

static tcp_t tcp_tbl[TCP_MAX_NR];

static mblock_t tcp_mblock;

static nlist_t tcp_list;

net_err_t tcp_init(void)
{
    plat_memset(tcp_tbl, 0, sizeof(tcp_tbl));

    nlist_init(&tcp_list);

    mblock_init(&tcp_mblock, tcp_tbl, sizeof(tcp_t), TCP_MAX_NR, NLOCKER_TYPE_NONE);

    dbug_info(DBG_MOD_TCP, "tcp init");
    return NET_ERR_OK;
}

static uint16_t tcp_alloc_port()
{
    for (uint16_t port = NET_PORT_DYN_START; port <= NET_PORT_DYN_END; ++port)
    {
        bool port_in_use = false;

        nlist_node_t* node;
        nlist_for_each(node, &tcp_list)
        {
            tcp_t* udp = nlist_entry(node, tcp_t, base.node);
            if (udp->base.local_port == port)
            {
                port_in_use = true;
                break;
            }
        }

        if (!port_in_use)
        {
            return port;
        }
    }
    return NET_PORT_EMPTY; // 没有可用端口
}

static uint32_t tcp_get_isn()
{
    seed_xoshiro(time(NULL));
    return xoshiro256ss();
}

static void tcp_init_connect(tcp_t* tcp)
{
    tcp->send.isn = tcp_get_isn();
    tcp->send.un_ack_seq = tcp->send.next_seq = tcp->send.isn;
    tcp->recv.next_seq = 0;
}

static net_err_t tcp_connect(sock_t* sock, const struct x_sockaddr* addr, x_socklen_t addrlen)
{
    tcp_t* tcp = (tcp_t*)sock;

    if (tcp->state != TCP_STATE_CLOSE)
    {
        dbug_error(DBG_MOD_TCP, "tcp_connect: invalid state %s", tcp_state_name(tcp->state));
        return NET_ERR_STATE;
    }

    const struct x_sockaddr_in* dest_addr = (const struct x_sockaddr_in*)addr;

    ipaddr_from_buf(&sock->remote_ip, dest_addr->sin_addr.addr_array);
    sock->remote_port = x_ntohs(dest_addr->sin_port);

    if (sock->local_port == NET_PORT_EMPTY)
    {
        sock->local_port = tcp_alloc_port();
        if (sock->local_port == NET_PORT_EMPTY)
        {
            dbug_error(DBG_MOD_TCP, "tcp_connect: no free port");
            return NET_ERR_FULL;
        }
    }

    if (ipaddr_is_any(&sock->local_ip))
    {
        // 查路由表
        route_entry_t* route = find_route_entry(&sock->remote_ip);
        if (route == NULL)
        {
            dbug_error(DBG_MOD_TCP, "tcp_connect: no route to remote ip");
            return NET_ERR_IP_UNREACH;
        }
        ipaddr_copy(&sock->local_ip, &route->netif->ipaddr);
    }

    net_err_t err;
    tcp_init_connect(tcp);

    if ((err = tcp_send_syn(tcp)) != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "tcp_connect: tcp_send_syn failed");
        return err;
    }
    tcp_set_state(tcp, TCP_STATE_SYN_SENT);

    return NET_ERR_NEED_WAIT;
}

void tcp_free(tcp_t* tcp)
{
    sock_wait_destroy(&tcp->conn.wait);
    sock_wait_destroy(&tcp->send.wait);
    sock_wait_destroy(&tcp->recv.wait);
    if (tcp->send.data)
    {
        free(tcp->send.data);
        tcp->send.data = NULL;
    }
    tcp->state = TCP_STATE_FREE;
    nlist_remove(&tcp_list, &tcp->base.node);
    mblock_free(&tcp_mblock, tcp);
}

static net_err_t tcp_close(sock_t* sock)
{
    tcp_t* tcp = (tcp_t*)sock;
    switch (tcp->state)
    {
    case TCP_STATE_CLOSE:
        tcp_free(tcp);
        return NET_ERR_OK;
    case TCP_STATE_SYN_SENT:
    case TCP_STATE_SYN_RECEIVED:
        break;
    case TCP_STATE_CLOSE_WAIT:
        tcp_send_fin(tcp);
        tcp_set_state(tcp, TCP_STATE_LAST_ACK);
        return NET_ERR_NEED_WAIT;
    case TCP_STATE_ESTABLISHED:
        tcp_send_fin(tcp);
        tcp_set_state(tcp, TCP_STATE_FIN_WAIT_1);
        return NET_ERR_NEED_WAIT;
    case TCP_STATE_FIN_WAIT_1:

    default:
        break;
    }
    return NET_ERR_OK;
}

static net_err_t tcp_send(sock_t* sock, const uint8_t* buf, const size_t len, const int flags, ssize_t* sent_size)
{
    tcp_t* tcp = (tcp_t*)sock;
    switch (tcp->state)
    {
    case TCP_STATE_CLOSE:
        return NET_ERR_CLOSE;
    case TCP_STATE_ESTABLISHED:

        break;
    default:
        dbug_error(DBG_MOD_TCP, "tcp_send: invalid state %s", tcp_state_name(tcp->state));
        return NET_ERR_STATE;
    }
    return NET_ERR_OK;
}

static tcp_t* tcp_get_free(const bool wait)
{
    tcp_t* tcp = mblock_alloc(&tcp_mblock, wait ? 0 : -1);
    if (tcp == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_get_free: no free tcp");
        return NULL;
    }
    return tcp;
}

tcp_t* tcp_find(const ipaddr_t* local_ip, const uint16_t local_port, const ipaddr_t* remote_ip,
                const uint16_t remote_port)
{
    nlist_node_t* node;
    nlist_for_each(node, &tcp_list)
    {
        tcp_t* tcp = nlist_entry(node, tcp_t, base.node);
        if (tcp->base.local_port == local_port &&
            (ipaddr_is_any(&tcp->base.local_ip) || ipaddr_is_equal(&tcp->base.local_ip, local_ip)) &&
            (ipaddr_is_any(&tcp->base.remote_ip) || ipaddr_is_equal(&tcp->base.remote_ip, remote_ip)) &&
            (tcp->base.remote_port == 0 || tcp->base.remote_port == remote_port))
        {
            return tcp;
        }
    }
    return NULL;
}

static tcp_t* tcp_alloc(const bool wait, const int family, const int protocol)
{
    tcp_t* tcp = tcp_get_free(wait);
    if (tcp == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: no memory for tcp");
        return NULL;
    }

    plat_memset(tcp, 0, sizeof(tcp_t));
    tcp_set_state(tcp, TCP_STATE_CLOSE);

    static const sock_ops_t tcp_ops = {
        .send = tcp_send,
        //.sendto = udp_sendto,
        // .recvfrom = udp_recvfrom,
        // .setopt = sock_setopt,
        .close = tcp_close,
        .connect = tcp_connect,
        // .recv = sock_recv,
        // .bind = udp_bind,
    };

    net_err_t err = sock_init(&tcp->base, family, protocol, &tcp_ops);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: sock_init failed");
        goto create_fail;
    }

    if (sock_wait_init(&tcp->conn.wait) != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: sock_wait_init failed");
        goto create_fail;
    }
    tcp->base.conn_wait = &tcp->conn.wait;

    if (sock_wait_init(&tcp->send.wait) != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: sock_wait_init failed");
        goto create_fail;
    }
    tcp->base.send_wait = &tcp->send.wait;

    if (sock_wait_init(&tcp->recv.wait) != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: sock_wait_init failed");
        goto create_fail;
    }
    tcp->base.recv_wait = &tcp->recv.wait;

    // 初始化发送缓冲区
    if (tcp->base.send_buf_size == 0)
    {
        tcp->base.send_buf_size = TCP_SEND_BUF_SIZE;
    }

    tcp->send.data = (uint8_t*)malloc(tcp->base.send_buf_size);
    tcp_buf_init(&tcp->send.buf, tcp->send.data, tcp->base.send_buf_size);

    return tcp;

create_fail:
    sock_free(&tcp->base);
    mblock_free(&tcp_mblock, tcp);
    if (tcp->base.conn_wait)
    {
        sock_wait_destroy(tcp->base.conn_wait);
    }
    if (tcp->base.send_wait)
    {
        sock_wait_destroy(tcp->base.send_wait);
    }
    if (tcp->base.recv_wait)
    {
        sock_wait_destroy(tcp->base.recv_wait);
    }
    return NULL;
}

static void tcp_insert(tcp_t* tcp)
{
    nlist_insert_last(&tcp_list, &tcp->base.node);
}

sock_t* tcp_create(const int family, const int protocol)
{
    tcp_t* tcp = tcp_alloc(true, family, protocol);
    if (tcp == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: tcp_alloc failed");
        return NULL;
    }
    tcp_insert(tcp);
    return &tcp->base;
}

size_t tcp_header_size(const tcp_header_t* header)
{
    return header->f_data_offset * 4;
}

void tcp_set_header_size(tcp_header_t* header, const size_t size)
{
    header->f_data_offset = size / 4;
}

net_err_t tcp_abort(tcp_t* tcp, const net_err_t err)
{
    tcp_set_state(tcp, TCP_STATE_CLOSE);
    sock_wakeup(&tcp->base, SOCK_WAIT_ALL, err);
    return NET_ERR_OK;
}
