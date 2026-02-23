#include "tcp_buf.h"

void tcp_buf_init(tcp_buf_t* buf, uint8_t* data, int size)
{
    buf->data = data;
    buf->size = size;
    buf->count = 0;
    buf->in = 0;
    buf->out = 0;
}

void tcp_buf_write(tcp_buf_t* buf, const uint8_t* data, int len)
{
    while (len > 0)
    {
        buf->data[buf->in++] = *data++;
        if (buf->in >= buf->size)
        {
            buf->in = 0;
        }
        buf->count++;
        len--;
    }
}

void tcp_buf_read(tcp_buf_t* buf, uint8_t* data, int len)
{
}
