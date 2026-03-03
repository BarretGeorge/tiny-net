#include "epoll.h"

int x_epoll_create1(int flags)
{
    return -1;
}

int x_epoll_ctl(int epfd, int op, int fd, x_epoll_event_t* event)
{
    return -1;
}

int x_epoll_wait(int epfd, x_epoll_event_t* events, int maxevents, int timeout)
{
    return -1;
}