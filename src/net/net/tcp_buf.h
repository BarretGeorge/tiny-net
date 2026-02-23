#ifndef TINY_NET_TCP_BUF_H
#define TINY_NET_TCP_BUF_H
#include <stdint.h>

typedef struct tcp_buf_t
{
    uint8_t* data;
    int count;
    int size;
    int in;
    int out;
} tcp_buf_t;

void tcp_buf_init(tcp_buf_t* buf, uint8_t* data, int size);

inline int tcp_buf_size(const tcp_buf_t* buf)
{
    return buf->size;
}

inline int tcp_buf_count(const tcp_buf_t* buf)
{
    return buf->count;
}

inline int tcp_buf_available(const tcp_buf_t* buf)
{
    return buf->size - buf->count;
}

#endif //TINY_NET_TCP_BUF_H
