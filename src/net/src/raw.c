#include "raw.h"
#include "dbug.h"
#include "mblock.h"
#include "ipv4.h"
#include "sock.h"
#include "socket.h"

static raw_t raw_tbl[RAW_MAX_NR];

static mblock_t raw_mblock;

static nlist_t raw_list;

net_err_t raw_init()
{
    plat_memset(raw_tbl, 0, sizeof(raw_tbl));

    nlist_init(&raw_list);

    mblock_init(&raw_mblock, raw_tbl, sizeof(raw_t), RAW_MAX_NR, NLOCKER_TYPE_NONE);
    return NET_ERR_OK;
}

static net_err_t raw_sendto(sock_t* sock, const uint8_t* buf, const size_t len, int flags,
                            const struct x_socketaddr* dest, x_socklen_t dest_len, ssize_t* sent_size)
{
    ipaddr_t dest_ip;

    struct x_sockaddr_in* addr = (struct x_sockaddr_in*)dest;
    ipaddr_from_buf(&dest_ip, addr->sin_addr.addr_array);

    if (!ipaddr_is_any(&sock->remote_ip) && !ipaddr_is_equal(&dest_ip, &sock->remote_ip))
    {
        dbug_error(DBG_MOD_RAW, "raw_sendto: destination address is any");
        return NET_ERR_INVALID_PARAM;
    }

    pktbuf_t* pktbuf = pktbuf_alloc((int)len);
    if (pktbuf == NULL)
    {
        dbug_error(DBG_MOD_RAW, "raw_sendto: pktbuf_alloc failed");
        return NET_ERR_MEM;
    }

    net_err_t err = pktbuf_write(pktbuf, buf, (int)len);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_RAW, "raw_sendto: pktbuf_write failed, err=%d", err);
        pktbuf_free(pktbuf);
        return err;
    }

    if (ipaddr_is_any(&sock->local_ip))
    {
        sock->local_ip = netif_get_default()->ipaddr;
    }

    err = ipv4_output(sock->protocol, &dest_ip, &sock->local_ip, pktbuf);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_RAW, "raw_sendto: ipv4_output failed, err=%d", err);
        pktbuf_free(pktbuf);
        return err;
    }
    *sent_size = (ssize_t)len;
    return NET_ERR_OK;
}

static net_err_t raw_recvfrom(sock_t* sock, uint8_t* buf, const size_t len, int flags,
                              const struct x_socketaddr* src, x_socklen_t* src_len, ssize_t* recv_size)
{
    *recv_size = 0;
    return NET_ERR_NEED_WAIT;
}

sock_t* raw_create(const int family, const int protocol)
{
    static const sock_ops_t raw_ops = {
        .sendto = raw_sendto,
        .recvfrom = raw_recvfrom,
        .setopt = sock_setopt,
    };
    raw_t* raw = mblock_alloc(&raw_mblock, -1);
    if (raw == NULL)
    {
        dbug_error(DBG_MOD_RAW, "raw_create: no memory for raw");
        return NULL;
    }

    net_err_t err = sock_init(&raw->base, family, protocol, &raw_ops);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_RAW, "raw_create: sock_init failed");
        goto create_fail;
    }

    raw->base.recv_wait = &raw->recv_wait;
    if ((err = sock_wait_init(&raw->recv_wait) != NET_ERR_OK))
    {
        dbug_error(DBG_MOD_RAW, "raw_create: sock_wait_init failed,err:%d", err);
        goto create_fail;
    }

    nlist_insert_last(&raw_list, &raw->base.node);

    return &raw->base;

create_fail:
    sock_free(&raw->base);
    mblock_free(&raw_mblock, raw);
    return NULL;
}
