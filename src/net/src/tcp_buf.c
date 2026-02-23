#include "tcp_buf.h"

#include "dbug.h"

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

void tcp_buf_read_send(const tcp_buf_t* buf, pktbuf_t* dest, const int offset, int size)
{
    // 超过要求的数据量，进行调整
    int free_for_us = buf->count - offset; // 跳过offset之前的数据
    if (size > free_for_us)
    {
        size = free_for_us;
    }

    // 复制过程中要考虑buf中的数据回绕的问题
    int start = buf->out + offset; // 注意拷贝的偏移
    if (start >= buf->size)
    {
        start -= buf->size;
    }

    while (size > 0)
    {
        // 当前超过末端，则只拷贝到末端的区域
        int end = start + size;
        if (end >= buf->size)
        {
            end = buf->size;
        }
        int copy_size = end - start;

        // 写入数据
        net_err_t err = pktbuf_write(dest, buf->data + start, copy_size);
        if (err != NET_ERR_OK)
        {
            dbug_error(DBG_MOD_TCP, "pktbuf_write failed: %d", err);
            return;
        }

        // 更新start，处理回绕的问题
        start += copy_size;
        if (start >= buf->size)
        {
            start -= buf->size;
        }
        size -= copy_size;
    }
}

int tcp_buf_remove(tcp_buf_t* buf, int len)
{
    if (len > buf->count)
    {
        len = buf->count;
    }
    buf->out += len;
    if (buf->out >= buf->size)
    {
        buf->out -= buf->size;
    }
    buf->count -= len;
    return len;
}
