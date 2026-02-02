#ifndef TINY_NET_SOCKET_H
#define TINY_NET_SOCKET_H

#include "ipv4.h"
#include "sock.h"

#undef  INADDR_ANY
#define INADDR_ANY         ((uint32_t)0x00000000)

#undef AF_INET
#define AF_INET            2

#undef SOCK_STREAM
#define SOCK_STREAM        1

#undef SOCK_DGRAM
#define SOCK_DGRAM         2

#undef SOCK_RAW
#define SOCK_RAW           3

#undef IPPROTO_ICMP
#define IPPROTO_ICMP       1

#pragma pack(1)
struct x_in_addr
{
    union
    {
        struct
        {
            uint8_t addr0;
            uint8_t addr1;
            uint8_t addr2;
            uint8_t addr3;
        };

        uint32_t s_addr;
        uint8_t addr_array[4];
    };
};

struct x_sockaddr
{
    uint8_t sa_len; // 整个结构的长度，值固定为16
    uint8_t sa_family; // 地址簇：NET_AF_INET
    uint8_t sa_data[14]; // 数据空间
};

struct x_sockaddr_in
{
    uint8_t sin_len; // 整个结构的长度，值固定为16
    uint8_t sin_family; // 地址簇：AF_INET
    uint16_t sin_port; // 端口号
    struct x_in_addr sin_addr; // IP地址
    char sin_zero[8]; // 填充字节
};

struct x_socketaddr
{
    uint8_t sa_len;
    uint8_t sa_family;
    char sa_data[14];
};
#pragma pack()

#define in_addr x_in_addr
#define socketaddr x_socketaddr
#define sockaddr x_sockaddr

int x_socket(int family, int type, int protocol);

ssize_t x_sendto(int fd, const void* buf, size_t len, int flags, struct x_socketaddr* addr, x_socklen_t addrlen);

ssize_t x_recvfrom(int fd, void* buf, size_t len, int flags, struct x_socketaddr* addr, x_socklen_t* addrlen);

int x_bind(int fd, const struct x_socketaddr* addr, unsigned int addrlen);
int x_listen(int fd, int backlog);
int x_accept(int fd, struct x_socketaddr* addr, unsigned int* addrlen);
int x_connect(int fd, const struct x_socketaddr* addr, unsigned int addrlen);
int x_send(int fd, const void* buf, unsigned int len, int flags);
int x_recv(int fd, void* buf, unsigned int len, int flags);
int x_close(int fd);
int x_setsockopt(int fd, int level, int optname, const void* optval, unsigned int optlen);

#endif //TINY_NET_SOCKET_H
