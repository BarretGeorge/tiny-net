#include "tcp_buf.h"

void tcp_buf_init(tcp_buf_t* buf, uint8_t* data, int size)
{
    buf->data = data;
    buf->size = size;
    buf->count = 0;
    buf->in = 0;
    buf->out = 0;
}
