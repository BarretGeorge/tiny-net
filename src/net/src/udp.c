#include "udp.h"
#include "dbug.h"
#include "mblock.h"
#include "socket.h"
#include "ipv4.h"
#include "tool.h"

static udp_t udp_tbl[UDP_MAX_NR];

static mblock_t udp_mblock;

static nlist_t udp_list;

static net_err_t alloc_port(sock_t* sock)
{
    for (uint16_t port = NET_PORT_DYN_START; port <= NET_PORT_DYN_END; ++port)
    {
        bool port_in_use = false;

        nlist_node_t* node;
        nlist_for_each(node, &udp_list)
        {
            udp_t* udp = nlist_entry(node, udp_t, base.node);
            if (udp->base.local_port == port)
            {
                port_in_use = true;
                break;
            }
        }

        if (!port_in_use)
        {
            sock->local_port = port;
            return NET_ERR_OK;
        }
    }

    return NET_ERR_FULL; // 没有可用端口
}

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

    // 分配端口
    if (sock->local_port == 0)
    {
        net_err_t err = alloc_port(sock);
        if (err != NET_ERR_OK)
        {
            dbug_error(DBG_MOD_UDP, "raw_sendto: get free udp port failed");
            return err;
        }
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

    err = upd_output(&dest_ip, dest_port, &sock->local_ip, sock->local_port, pktbuf);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_UDP, "raw_sendto: ipv4_output failed, err=%d", err);
        pktbuf_free(pktbuf);
        return err;
    }
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

net_err_t upd_output(const ipaddr_t* dest_ip, const uint16_t dest_port, const ipaddr_t* src_ip, const uint16_t src_port,
                     pktbuf_t* buf)
{
    net_err_t err = pktbuf_add_header(buf, sizeof(udp_header_t), true);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_UDP, "upd_output: pktbuf_add_header failed, err=%d", err);
        return err;
    }
    udp_header_t* udp_hdr = (udp_header_t*)pktbuf_data(buf);
    udp_hdr->src_port = x_htons(src_port);
    udp_hdr->dest_port = x_htons(dest_port);
    udp_hdr->length = x_htons(buf->total_size);
    udp_hdr->checksum = 0;
    udp_hdr->checksum = checksum16_pseudo(buf, src_ip, dest_ip, IPPROTO_UDP);

    err = ipv4_output(IPPROTO_UDP, dest_ip, src_ip, buf);
    if (err != NET_ERR_OK)
    {
        dbug_error(DBG_MOD_UDP, "upd_output: ipv4_output failed, err=%d", err);
        return err;
    }
    return NET_ERR_OK;
}
