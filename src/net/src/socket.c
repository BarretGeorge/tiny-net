#include "socket.h"
#include "exmsg.h"
#include "sock.h"

int x_socket(const int family, const int type, const int protocol)
{
    sock_req_t req;
    req.fd = -1;
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
