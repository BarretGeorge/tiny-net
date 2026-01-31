#include "socket.h"

int x_socket(int domain, int type, int protocol)
{
    return 0;
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
    return len;
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
