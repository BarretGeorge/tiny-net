#ifndef TINY_NET_MBLOCK_H
#define TINY_NET_MBLOCK_H

#include "nlist.h"
#include "nlocker.h"

typedef struct mblock_t
{
    nlist_t free_list; // 空闲列表
    void* start; // 内存块起始地址
    nlocker_t lock; // 锁
    sys_sem_t alloc_sem; // 分配信号量
} mblock_t;

net_err_t mblock_init(mblock_t* mblock, void* mem, size_t block_size, size_t cnt, nlocker_type_t type);

void* mblock_alloc(mblock_t* mblock, int32_t timeout_ms);

int mblock_free_cnt(const mblock_t* mblock);

void mblock_free(mblock_t* mblock, void* block);

void mblock_destroy(const mblock_t* mblock);

#endif //TINY_NET_MBLOCK_H
