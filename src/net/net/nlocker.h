#ifndef TINY_NET_NLOCKER_H
#define TINY_NET_NLOCKER_H

#include "sys.h"
#include "net_err.h"

typedef enum nlocker_type_t
{
    NLOCKER_TYPE_NONE = 0, // 无锁
    NLOCKER_TYPE_THREAD, // 线程锁
} nlocker_type_t;

typedef struct nlocker_t
{
    nlocker_type_t type; // 锁类型
    union
    {
        sys_mutex_t mutex; // 互斥锁
    };
} nlocker_t;

net_err_t nlocker_init(nlocker_t* locker, nlocker_type_t type);

void nlocker_destroy(const nlocker_t* locker);

void nlocker_lock(const nlocker_t* locker);

void nlocker_unlock(const nlocker_t* locker);

#endif //TINY_NET_NLOCKER_H
