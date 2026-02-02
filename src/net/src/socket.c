#include "socket.h"
#include "dbug.h"
#include "exmsg.h"
#include "sock.h"

int x_socket(const int family, const int type, const int protocol)
{
    sock_req_t req;
    req.fd = -1;
    req.wait = NULL;
    req.wait_timeout = 0;
    req.create.family = family;
    req.create.type = type;
    req.create.protocol = protocol;
    net_err_t err = exmsg_func_exec(socket_create_req_in, &req);
    if (err != NET_ERR_OK)
    {
        return -1;
    }
    return req.fd;
}

ssize_t x_sendto(const int fd, const void* buf, size_t len, const int flags, struct x_socketaddr* addr,
                 x_socklen_t addrlen)
{
    if (buf == NULL || addr == NULL || len == 0)
    {
        return -1;
    }

    if (addr->sa_family != AF_INET || addrlen != sizeof(struct sockaddr))
    {
        return -1;
    }

    uint8_t* send_buf = (uint8_t*)buf;
    ssize_t total_sent = 0;
    while (len > 0)
    {
        sock_req_t req;
        req.fd = fd;
        req.data.addrlen = &addrlen;
        req.data.addr = addr;
        req.data.buf = send_buf;
        req.data.len = len;
        req.data.flags = flags;
        req.wait = NULL;
        req.wait_timeout = 0;

        net_err_t err = exmsg_func_exec(socket_sendto_req_in, &req);
        if (err < NET_ERR_OK)
        {
            dbug_error(DBG_MOD_SOCKET, "socket_sendto_req_in sendto failed");
            return -1;
        }

        if (req.wait && (err = sock_wait_enter(req.wait, req.wait_timeout)) < NET_ERR_OK)
        {
            dbug_error(DBG_MOD_SOCKET, "socket_sendto: wait failed, err=%d", err);
            return -1;
        }

        len -= req.data.transferred_len;
        send_buf += req.data.transferred_len;
        total_sent += req.data.transferred_len;
    }
    return total_sent;
}

ssize_t x_recvfrom(const int fd, void* buf, const size_t len, const int flags, struct x_socketaddr* addr,
                   x_socklen_t* addrlen)
{
    if (buf == NULL || addr == NULL || len == 0 || addrlen == NULL)
    {
        return -1;
    }

    while (true)
    {
        sock_req_t req;
        req.wait = NULL;
        req.wait_timeout = 0;
        req.fd = fd;
        req.data.addrlen = addrlen;
        req.data.addr = addr;
        req.data.buf = buf;
        req.data.len = len;
        req.data.flags = flags;
        req.data.transferred_len = 0;

        net_err_t err = exmsg_func_exec(socket_recvfrom_req_in, &req);
        if (err < NET_ERR_OK)
        {
            dbug_error(DBG_MOD_SOCKET, "socket_recvfrom_req_in recvfrom failed");
            return -1;
        }
        if (req.data.transferred_len > 0)
        {
            return req.data.transferred_len;
        }

        // 等待数据到达
        err = sock_wait_enter(req.wait, req.wait_timeout);
        if (err < NET_ERR_OK)
        {
            dbug_error(DBG_MOD_SOCKET, "socket_recvfrom: wait failed, err=%d", err);
            return err;
        }
    }
}

int x_bind(int fd, const struct x_socketaddr* addr, unsigned int addrlen)
{
    return 0;
}

int x_listen(int fd, int backlog)
{
    return 0;
}

int x_accept(int fd, struct x_socketaddr* addr, unsigned int* addrlen)
{
    return 0;
}

int x_connect(int fd, const struct x_socketaddr* addr, unsigned int addrlen)
{
    return 0;
}

int x_send(int fd, const void* buf, unsigned int len, int flags)
{
    return 0;
}

int x_recv(int fd, void* buf, unsigned int len, int flags)
{
    return 0;
}

int x_close(int fd)
{
    return 0;
}

int x_setsockopt(int fd, int level, int optname, const void* optval, unsigned int optlen)
{
    return 0;
}
