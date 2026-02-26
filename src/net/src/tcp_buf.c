#include "tcp_buf.h"

#include "dbug.h"
#include "sys_plat.h"

void tcp_buf_init(tcp_buf_t* buf, uint8_t* data, const int size)
{
    buf->data = data;
    buf->size = size;
    buf->count = 0;
    buf->in = 0;
    buf->out = 0;
}

void tcp_buf_write_send(tcp_buf_t* buf, const uint8_t* data, int len)
{
    while (len > 0)
    {
        // 计算从in到缓冲区末尾的可写长度
        int free_to_end = buf->size - buf->in;
        int curr_copy = len > free_to_end ? free_to_end : len;

        plat_memcpy(buf->data + buf->in, data, curr_copy);
        buf->in += curr_copy;
        if (buf->in >= buf->size)
        {
            buf->in = 0;
        }
        buf->count += curr_copy;
        data += curr_copy;
        len -= curr_copy;
    }
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

int tcp_buf_write_recv(tcp_buf_t* dest, const int offset, pktbuf_t* pkt, int len)
{
    // 计算缓冲区中的起始索引，注意回绕
    int start = dest->in + offset;
    if (start >= dest->size)
    {
        start = start - dest->size;
    }

    // 计算实际可写的数据量
    int free_size = tcp_buf_available(dest) - offset; // 跳过的一部分相当于是已经被写入了
    len = len > free_size ? free_size : len;

    int size = len;
    while (size > 0)
    {
        // 从start到缓存末端的单元数量，可能其中有数据也可能没有
        int free_to_end = dest->size - start;

        // 大小超过到尾部的空闲数据量，只拷贝一部分
        int curr_copy = size > free_to_end ? free_to_end : size;
        pktbuf_read(pkt, dest->data + start, (int)curr_copy);

        // 增加写索引，注意回绕
        start += curr_copy;
        if (start >= dest->size)
        {
            start = start - dest->size;
        }

        // 增加已写入的数据量
        dest->count += curr_copy;
        size -= curr_copy;
    }

    dest->in = start;
    return len;
}

int tcp_buf_read_recv(tcp_buf_t* src, uint8_t* buf, const int len)
{
    // 选实际能读取的量
    int total = len > src->count ? src->count : len;

    int curr_size = 0;
    while (curr_size < total)
    {
        // 计算从out到缓冲区末尾的可读长度
        int free_to_end = src->size - src->out;
        int remain = total - curr_size;
        int curr_copy = remain > free_to_end ? free_to_end : remain;

        plat_memcpy(buf, src->data + src->out, curr_copy);
        src->out += curr_copy;
        if (src->out >= src->size)
        {
            src->out = 0;
        }
        src->count -= curr_copy;
        buf += curr_copy;
        curr_size += curr_copy;
    }
    return total;
}
