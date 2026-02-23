#ifndef TINY_NET_TCP_BUF_H
#define TINY_NET_TCP_BUF_H

#include <stdint.h>
#include "pktbuf.h"

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

static inline int tcp_buf_count(const tcp_buf_t* buf)
{
    return buf->count;
}

static inline int tcp_buf_available(const tcp_buf_t* buf)
{
    return buf->size - buf->count;
}

void tcp_buf_write(tcp_buf_t* buf, const uint8_t* data, int len);

void tcp_buf_read(tcp_buf_t* buf, uint8_t* data, int len);

void tcp_buf_read_send(const tcp_buf_t* buf, pktbuf_t* dest, int offset, int size);

int tcp_buf_remove(tcp_buf_t* buf, int len);

#endif //TINY_NET_TCP_BUF_H
