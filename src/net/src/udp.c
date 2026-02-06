#include "udp.h"
#include "dbug.h"
#include "mblock.h"
#include "socket.h"
#include "ipv4.h"
#include "tool.h"

static udp_t udp_tbl[UDP_MAX_NR];

static mblock_t udp_mblock;

static nlist_t udp_list;

net_err_t upd_init()
{
    plat_memset(udp_tbl, 0, sizeof(udp_tbl));

    nlist_init(&udp_list);

    mblock_init(&udp_mblock, udp_tbl, sizeof(udp_t), UDP_MAX_NR, NLOCKER_TYPE_NONE);

    dbug_info(DBG_MOD_UDP, "init udp");
    return NET_ERR_OK;
}

static net_err_t udp_sendto(sock_t* sock, const uint8_t* buf, const size_t len, int flags,
                            const struct x_socketaddr* dest, x_socklen_t dest_len, ssize_t* sent_size)
{
    ipaddr_t dest_ip;

    struct x_sockaddr_in* addr = (struct x_sockaddr_in*)dest;
    ipaddr_from_buf(&dest_ip, addr->sin_addr.addr_array);

    uint16_t dest_port = x_ntohs(addr->sin_port);
    if (sock->remote_port != 0 && sock->remote_port != dest_port)
    {
        dbug_error(DBG_MOD_UDP, "raw_sendto: destination port mismatch");
        return NET_ERR_INVALID_PARAM;
    }

    if (!ipaddr_is_any(&sock->remote_ip) && !ipaddr_is_equal(&dest_ip, &sock->remote_ip))
    {
        dbug_error(DBG_MOD_UDP, "raw_sendto: destination address is any");
        return NET_ERR_INVALID_PARAM;
    }

    pktbuf_t* pktbuf = pktbuf_alloc((int)len);
    if (pktbuf == NULL)
    {
        dbug_error(DBG_MOD_UDP, "raw_sendto: pktbuf_alloc failed");
        return NET_ERR_MEM;
    }

    net_err_t err = pktbuf_write(pktbuf, buf, (int)len);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_UDP, "raw_sendto: pktbuf_write failed, err=%d", err);
        pktbuf_free(pktbuf);
        return err;
    }

    if (ipaddr_is_any(&sock->local_ip))
    {
        sock->local_ip = netif_get_default()->ipaddr;
    }


    // err = ipv4_output(sock->protocol, &dest_ip, &sock->local_ip, pktbuf);
    // if (err != NET_ERR_OK)
    // {
    //     dbug_error(DBG_MOD_UDP, "raw_sendto: ipv4_output failed, err=%d", err);
    //     pktbuf_free(pktbuf);
    //     return err;
    // }
    *sent_size = (ssize_t)len;
    return NET_ERR_OK;
}

static net_err_t udp_recvfrom(sock_t* sock, uint8_t* buf, const size_t len, int flags,
                              const struct x_socketaddr* src, x_socklen_t* src_len, ssize_t* recv_size)
{
    return NET_ERR_OK;
}

static net_err_t udp_close(sock_t* sock)
{
    udp_t* udp = (udp_t*)sock;

    // 从全局列表中移除
    nlist_remove(&udp_list, &udp->base.node);

    // 移除消息队列中的数据包
    nlist_node_t* node;
    while ((node = nlist_remove_first(&udp->recv_list)) != NULL)
    {
        pktbuf_t* pktbuf = nlist_entry(node, pktbuf_t, node);
        pktbuf_free(pktbuf);
    }

    // 释放sock
    sock_free(sock);

    // 释放udp内存块
    mblock_free(&udp_mblock, udp);
    return NET_ERR_OK;
}

sock_t* udp_create(const int family, const int protocol)
{
    static const sock_ops_t udp_ops = {
        .sendto = udp_sendto,
        .recvfrom = udp_recvfrom,
        .setopt = sock_setopt,
        .close = udp_close,
    };
    udp_t* udp = mblock_alloc(&udp_mblock, -1);
    if (udp == NULL)
    {
        dbug_error(DBG_MOD_UDP, "udp_create: no memory for raw");
        return NULL;
    }

    net_err_t err = sock_init(&udp->base, family, protocol, &udp_ops);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_UDP, "udp_create: sock_init failed");
        goto create_fail;
    }

    udp->base.recv_wait = &udp->recv_wait;
    if ((err = sock_wait_init(&udp->recv_wait)) != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_UDP, "udp_create: sock_wait_init failed,err:%d", err);
        goto create_fail;
    }

    nlist_insert_last(&udp_list, &udp->base.node);
    nlist_init(&udp->recv_list);

    return &udp->base;

create_fail:
    sock_free(&udp->base);
    mblock_free(&udp_mblock, udp);
    return NULL;
}
