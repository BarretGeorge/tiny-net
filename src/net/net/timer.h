#ifndef TINY_NET_TIMER_H
#define TINY_NET_TIMER_H

#include "net_cfg.h"
#include "net_err.h"
#include "nlist.h"
#include <stdint.h>

// 周期性定时器标志
#define TIMER_FLAG_PERIODIC      0x01

struct net_timer_t;

typedef void (*timer_proc_t)(struct net_timer_t* timer, void* arg);

typedef struct net_timer_t
{
    // 链表结点
    nlist_node_t node;
    // 名称
    char name[TIMER_NAME_LEN];
    // 标志位
    int flags;
    // 定时器到期时间，单位：毫秒
    uint32_t expire;
    // 周期时间，单位：毫秒，0表示非周期定时器
    uint32_t interval;
    // 定时器回调函数
    timer_proc_t proc;
    // 回调函数参数
    void* arg;
} net_timer_t;

net_err_t net_timer_init();

net_err_t net_timer_add(net_timer_t* timer,
                        const char* name,
                        timer_proc_t proc,
                        void* arg,
                        uint32_t ms,
                        int flags
);

net_err_t net_timer_remove(const net_timer_t* timer);

uint32_t net_timer_check_mo(uint32_t diff_ms);

#endif //TINY_NET_TIMER_H
