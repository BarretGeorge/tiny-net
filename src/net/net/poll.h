#ifndef TINY_NET_POLL_H
#define TINY_NET_POLL_H

#include "sock.h"

// poll 事件标志
#define POLLIN    0x0001  // 可读
#define POLLOUT   0x0004  // 可写
#define POLLERR   0x0008  // 错误
#define POLLHUP   0x0010  // 挂断
#define POLLNVAL  0x0020  // 无效 fd

typedef struct x_pollfd
{
    int fd;
    short events; // 请求的事件
    short revents; // 返回的事件
} x_pollfd_t;

typedef struct sock_poll_t
{
    x_pollfd_t* fds;
    unsigned int nfds;
    sock_wait_t* poll_wait; // 共享信号量，任一 socket 就绪时通知
    int ready_count; // 输出: 就绪 fd 数量
    int phase; // 0=check+register, 1=check_only, 2=unregister
} sock_poll_t;

int x_poll(x_pollfd_t* fds, unsigned int nfds, int timeout);

net_err_t socket_poll_req_in(const func_msg_t* msg);

#define pollfd x_pollfd

#define poll x_poll

#endif //TINY_NET_POLL_H
