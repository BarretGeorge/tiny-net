#include "sock.h"
#include "dbug.h"
#include "raw.h"
#include "udp.h"
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
        [SOCK_DGRAM] = {.create = udp_create, .protocol = IPPROTO_UDP},
        [SOCK_STREAM] = {.create = NULL, .protocol = IPPROTO_TCP},
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
    s->sock = info.create(req->create.family, req->create.protocol);
    if (s->sock == NULL)
    {
        socket_free(s);
        return NET_ERR_MEM;
    }

    req->fd = socket_get_fd(s);
    return NET_ERR_OK;
}

net_err_t socket_sendto_req_in(const func_msg_t* msg)
{
    sock_req_t* req = msg->arg;
    sock_data_t* data = &req->data;
    x_socket_t* s = socket_get(req->fd);
    if (s == NULL)
    {
        return NET_ERR_INVALID_PARAM;
    }
    if (s->sock->ops->sendto == NULL)
    {
        return NET_ERR_INVALID_STATE;
    }
    net_err_t err = s->sock->ops->sendto(s->sock, data->buf, data->len, data->flags, data->addr, *data->addrlen,
                                         &data->transferred_len);
    if (err == NET_ERR_NEED_WAIT && s->sock->send_wait)
    {
        sock_wait_add(s->sock->send_wait, s->sock->send_timeout, req);
    }
    if (err < NET_ERR_OK)
    {
        dbug_error(DBG_MOD_SOCK, "socket sendto failed, err=%d", err);
        return err;
    }
    return NET_ERR_OK;
}

net_err_t socket_recvfrom_req_in(const func_msg_t* msg)
{
    sock_req_t* req = msg->arg;
    sock_data_t* data = &req->data;
    x_socket_t* s = socket_get(req->fd);
    if (s == NULL)
    {
        return NET_ERR_INVALID_PARAM;
    }
    if (s->sock->ops->recvfrom == NULL)
    {
        return NET_ERR_INVALID_STATE;
    }
    net_err_t err = s->sock->ops->recvfrom(s->sock, data->buf, data->len, data->flags, data->addr, data->addrlen,
                                           &data->transferred_len);
    if (err == NET_ERR_NEED_WAIT && s->sock->recv_wait)
    {
        sock_wait_add(s->sock->recv_wait, s->sock->recv_timeout, req);
    }

    if (err < NET_ERR_OK)
    {
        dbug_error(DBG_MOD_SOCK, "socket recvfrom failed, err=%d", err);
        return err;
    }
    return NET_ERR_OK;
}

net_err_t socket_setsockopt_req_in(const func_msg_t* msg)
{
    sock_req_t* req = msg->arg;
    x_socket_t* s = socket_get(req->fd);
    if (s == NULL)
    {
        return NET_ERR_INVALID_PARAM;
    }
    // switch (req->opt.opt_name)
    // {
    // case SO_RCVTIMEO:
    //     break;
    // case SO_SNDTIMEO:
    //     break;
    // default:
    //     return NET_ERR_OPTION;
    // }
    return s->sock->ops->setopt(s->sock, req->opt.level, req->opt.opt_name, req->opt.opt_val, req->opt.opt_len);
}

net_err_t socket_close_req_in(const func_msg_t* msg)
{
    sock_req_t* req = msg->arg;
    x_socket_t* s = socket_get(req->fd);
    if (s == NULL)
    {
        return NET_ERR_INVALID_PARAM;
    }
    net_err_t err = NET_ERR_OK;
    if (s->sock->ops->close)
    {
        err = s->sock->ops->close(s->sock);
    }
    // sock_free(s->sock);
    socket_free(s);
    return err;
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
    sock->send_wait = NULL;
    sock->recv_wait = NULL;
    sock->conn_wait = NULL;
    return NET_ERR_OK;
}

net_err_t sock_wait_init(sock_wait_t* wait)
{
    wait->waiting = 0;
    wait->err = NET_ERR_OK;
    wait->sem = sys_sem_create(0);
    if (wait->sem == SYS_SEM_INVALID)
    {
        return NET_ERR_SYS;
    }
    return NET_ERR_OK;
}

void sock_wait_destroy(sock_wait_t* wait)
{
    if (wait->sem != SYS_SEM_INVALID)
    {
        sys_sem_free(wait->sem);
        wait->sem = SYS_SEM_INVALID;
    }
}

void sock_wait_add(sock_wait_t* wait, const int timeout, sock_req_t* req)
{
    wait->waiting++;

    req->wait = wait;
    req->wait_timeout = timeout;
}

net_err_t sock_wait_enter(const sock_wait_t* wait, const int timeout)
{
    if (sys_sem_wait(wait->sem, timeout) < 0)
    {
        return NET_ERR_TIMEOUT;
    }
    return wait->err;
}

void sock_wait_leave(sock_wait_t* wait, const net_err_t err)
{
    if (wait->waiting > 0)
    {
        wait->waiting--;
        wait->err = err;
        sys_sem_notify(wait->sem);
    }
}

void sock_wakeup(const sock_t* sock, const int type, const net_err_t err)
{
    if (type & SOCK_WAIT_READ && sock->recv_wait)
    {
        sock_wait_leave(sock->recv_wait, err);
    }
    if (type & SOCK_WAIT_WRITE && sock->send_wait)
    {
        sock_wait_leave(sock->send_wait, err);
    }
    if (type & SOCK_WAIT_CONN && sock->conn_wait)
    {
        sock_wait_leave(sock->conn_wait, err);
    }
}

void sock_free(const sock_t* sock)
{
    if (sock == NULL)
    {
        return;
    }
    if (sock->recv_wait)
    {
        sock_wait_destroy(sock->recv_wait);
    }
    if (sock->send_wait)
    {
        sock_wait_destroy(sock->send_wait);
    }
    if (sock->conn_wait)
    {
        sock_wait_destroy(sock->conn_wait);
    }
}

net_err_t sock_setopt(sock_t* sock, int level, int opt_name, const void* opt_val, int opt_len)
{
    if (level != SOL_SOCKET)
    {
        return NET_ERR_OPTION;
    }

    switch (opt_name)
    {
    case SO_RCVTIMEO:
        if (opt_len != sizeof(x_timeval))
        {
            return NET_ERR_INVALID_PARAM;
        }
        x_timeval tv = *(const x_timeval*)opt_val;
        sock->recv_timeout = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
        break;
    case SO_SNDTIMEO:
        if (opt_len != sizeof(x_timeval))
        {
            return NET_ERR_INVALID_PARAM;
        }
        x_timeval stv = *(const x_timeval*)opt_val;
        sock->send_timeout = (stv.tv_sec * 1000) + (stv.tv_usec / 1000);
        break;
    default:
        return NET_ERR_OPTION;
    }
    return NET_ERR_OK;
}
