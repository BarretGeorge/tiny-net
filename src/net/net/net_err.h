#ifndef TINY_NET_NET_ERR_H
#define TINY_NET_NET_ERR_H

typedef enum net_err_t
{
    NET_ERR_OK = 0,
    NET_ERR_SYS = -1, // 系统错误
    NET_ERR_MEM = -2, // 内存错误
} net_err_t;

#endif //TINY_NET_NET_ERR_H
