#ifndef TINY_NET_FIXQ_H
#define TINY_NET_FIXQ_H

#include "nlocker.h"
#include "sys.h"

typedef struct fixq_t
{
    int size; // 队列大小
    int in; // 写入索引
    int out; // 读取索引
    int cnt; // 当前队列中的元素个数
    void** buf; // 元素缓冲区
    nlocker_t locker; // 互斥锁
    sys_sem_t recv_sem; // 数据可用信号量
    sys_sem_t send_sem; // 空间可用信号量
} fixq_t;

net_err_t fixq_init(fixq_t* q, void** buf, int size, nlocker_type_t share_type);

void fixq_destroy(fixq_t* q);

#endif //TINY_NET_FIXQ_H
