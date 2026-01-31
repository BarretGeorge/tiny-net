#include "socket.h"

int x_socket(int domain, int type, int protocol)
{
    return 0;
}

int x_bind(int fd, const struct x_socketaddr* addr, unsigned int addrlen);
int x_listen(int fd, int backlog);
int x_accept(int fd, struct x_socketaddr* addr, unsigned int* addrlen);
int x_connect(int fd, const struct x_socketaddr* addr, unsigned int addrlen);
int x_send(int fd, const void* buf, unsigned int len, int flags);
int x_recv(int fd, void* buf, unsigned int len, int flags);
int x_close(int fd);
