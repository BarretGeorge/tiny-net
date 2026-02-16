#include "tcp.h"
#include "dbug.h"
#include "mblock.h"

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

static tcp_t* tcp_alloc(const bool wait, const int family, const int protocol)
{
    tcp_t* tcp = tcp_get_free(wait);
    if (tcp == NULL)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: no memory for tcp");
        return NULL;
    }

    plat_memset(tcp, 0, sizeof(tcp_t));

    static const sock_ops_t tcp_ops = {
        // .sendto = udp_sendto,
        // .recvfrom = udp_recvfrom,
        // .setopt = sock_setopt,
        // .close = udp_close,
        // .connect = sock_connect,
        // .send = sock_send,
        // .recv = sock_recv,
        // .bind = udp_bind,
    };

    net_err_t err = sock_init(&tcp->base, family, protocol, &tcp_ops);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_TCP, "tcp_create: sock_init failed");
        goto create_fail;
    }

    return tcp;

create_fail:
    sock_free(&tcp->base);
    mblock_free(&tcp_mblock, tcp);
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
    return header->data_offset * 4;
}

void tcp_set_header_size(tcp_header_t* header, const size_t size)
{
    header->data_offset = size / 4;
}
