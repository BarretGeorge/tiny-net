#ifndef TINY_NET_PKTBUF_H
#define TINY_NET_PKTBUF_H

#include "net_cfg.h"
#include "net_err.h"
#include "nlist.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct pktblk_t
{
    nlist_node_t node;
    uint8_t* data;
    uint8_t payload[PKTBUF_PAYLOAD_SIZE];
    uint32_t size;
} pktblk_t;

typedef struct pktbuf_t
{
    uint32_t total_size;
    nlist_t blk_list;
    nlist_node_t node;
    int pos;
    pktblk_t* curr_blk;
    uint8_t* blk_offset;
    int ref_count;
} pktbuf_t;

static pktblk_t* pktbuf_first_blk(const pktbuf_t* pktbuf)
{
    nlist_node_t* first = nlist_first(&pktbuf->blk_list);
    if (first == NULL)
    {
        return NULL;
    }
    return nlist_entry(first, pktblk_t, node);
}

static uint8_t* pktbuf_data(const pktbuf_t* pktbuf)
{
    const pktblk_t* first = pktbuf_first_blk(pktbuf);
    return first ? first->data : NULL;
}

static inline pktblk_t* pktblock_get_next(const pktblk_t* block)
{
    nlist_node_t* next = nlist_node_next(&block->node);
    return nlist_entry(next, pktblk_t, node);
}

static inline pktblk_t* pktblock_get_prev(const pktblk_t* block)
{
    nlist_node_t* prev = nlist_node_prev(&block->node);
    return nlist_entry(prev, pktblk_t, node);
}

static inline int pktbuf_total(const pktbuf_t* pktbuf)
{
    return pktbuf ? (int)pktbuf->total_size : 0;
}

// 初始化pktbuf模块
net_err_t pktbuf_init();

// 分配pktbuf
pktbuf_t* pktbuf_alloc(int size);

// 释放pktbuf
void pktbuf_free(pktbuf_t* pktbuf);

// 添加包头
net_err_t pktbuf_add_header(pktbuf_t* pktbuf, int size, bool is_cont);

// 移除包头
net_err_t pktbuf_remove_header(pktbuf_t* pktbuf, int size);

// 调整包大小
net_err_t pktbuf_resize(pktbuf_t* pktbuf, int new_size);

// 拼接两个包
net_err_t join_pktbuf(pktbuf_t* dst, pktbuf_t* src);

// 设置包头连续
net_err_t pktbuf_set_cont(pktbuf_t* pktbuf, int size);

// 重置访问位置
void pktbuf_reset_access(pktbuf_t* pktbuf);

// 对pktbuf中写入数据
net_err_t pktbuf_write(pktbuf_t* pktbuf, const uint8_t* buf, int size);

// 从pktbuf中读数据
net_err_t pktbuf_read(pktbuf_t* pktbuf, uint8_t* buf, int size);

// 从pktbuf中窥探数据，不移动访问位置
net_err_t pktbuf_peek(pktbuf_t* pktbuf, uint8_t* buf, int size, int offset);

// 移动访问位置
net_err_t pktbuf_seek(pktbuf_t* pktbuf, int offset);

// 复制pktbuf数据
net_err_t pktbuf_copy(pktbuf_t* dst, pktbuf_t* src, int size);

// 用指定值填充pktbuf
net_err_t pktbuf_fill(pktbuf_t* pktbuf, uint8_t value, int size);

// 增加引用计数
void pktbuf_incr_ref(pktbuf_t* pktbuf);

#endif // TINY_NET_PKTBUF_H
