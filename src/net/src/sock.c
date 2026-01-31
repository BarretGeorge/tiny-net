#include "sock.h"
#include "raw.h"
#include "sys_plat.h"
#include "socket.h"

static x_socket_t socket_tbl[SOCKET_MAX_FD];

static int socket_get_fd(const x_socket_t* sock)
{
    return (int)(sock - socket_tbl);
}

static x_socket_t* socket_get(const int fd)
{
    if (fd < 0 || fd >= SOCKET_MAX_FD)
    {
        return NULL;
    }
    x_socket_t* sock = &socket_tbl[fd];
    if (sock->state != SOCK_STATE_USED)
    {
        return NULL;
    }
    return sock;
}

static x_socket_t* socket_alloc()
{
    for (int i = 0; i < SOCKET_MAX_FD; ++i)
    {
        if (socket_tbl[i].state == SOCK_STATE_FREE)
        {
            socket_tbl[i].state = SOCK_STATE_USED;
            return &socket_tbl[i];
        }
    }
    return NULL;
}

static void socket_free(x_socket_t* sock)
{
    if (sock)
    {
        sock->state = SOCK_STATE_FREE;
    }
}

net_err_t socket_init(void)
{
    plat_memset(socket_tbl, 0, sizeof(socket_tbl));
    return NET_ERR_OK;
}

net_err_t socket_create_req_in(const func_msg_t* msg)
{
    static const struct sock_info_t
    {
        sock_t* (*create)(int family, int protocol);
        int protocol;
    } sock_tbl[] = {
        [SOCK_RAW] = {.create = raw_create, .protocol = IPPROTO_ICMP},
    };

    sock_req_t* req = msg->arg;
    x_socket_t* s = socket_alloc();
    if (s == NULL)
    {
        return NET_ERR_MEM;
    }

    if (req->create.type < 0 || req->create.type >= (int)(sizeof(sock_tbl) / sizeof(sock_tbl[0])))
    {
        socket_free(s);
        return NET_ERR_PROTOCOL;
    }

    struct sock_info_t info = sock_tbl[req->create.type];
    if (info.create == NULL)
    {
        socket_free(s);
        return NET_ERR_PROTOCOL;
    }
    if (req->create.protocol == 0)
    {
        req->create.protocol = info.protocol;
    }
    sock_t* sock = info.create(req->create.family, req->create.protocol);

    req->fd = socket_get_fd(s);
    return NET_ERR_OK;
}

net_err_t sock_init(sock_t* sock, const int family, const int protocol, const sock_ops_t* ops)
{
    sock->family = family;
    sock->protocol = protocol;
    sock->ops = ops;
    sock->err = NET_ERR_OK;
    ipaddr_set_any(&sock->local_ip);
    ipaddr_set_any(&sock->remote_ip);
    nlist_node_init(&sock->node);
    sock->local_port = 0;
    sock->remote_port = 0;
    sock->recv_timeout = 0;
    sock->send_timeout = 0;
    return NET_ERR_OK;
}
