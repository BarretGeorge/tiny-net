#include "sock.h"

#include "sys_plat.h"

#define SOCKET_MAX_FD 64

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
    sock_req_t* req = (sock_req_t*)msg;
    x_socket_t* sock = socket_alloc();
    if (sock == NULL)
    {
        return NET_ERR_MEM;
    }
    sock->domain = req->create.domain;
    sock->type = req->create.type;
    sock->protocol = req->create.protocol;
    req->fd = socket_get_fd(sock);
    return NET_ERR_OK;
}
