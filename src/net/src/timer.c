#include "timer.h"
#include "dbug.h"
#include "sys_plat.h"

static nlist_t timer_list;

net_err_t net_timer_init()
{
    dbug_info("Initializing timer module...");
    nlist_init(&timer_list);
    return NET_ERR_OK;
}

#if DBG_DISPLAY_ENABLE(DBG_BUG)
static void display_timer()
{
    dbug_info("--------------timer--start------------");
    nlist_node_t* node;
    dbug_info("Current timers:");
    nlist_for_each(node, &timer_list)
    {
        net_timer_t* timer = nlist_entry(node, net_timer_t, node);
        dbug_info("  Timer name: %s, expire:%u, interval: %u ms, flags: 0x%02X",
                  timer->name, timer->expire, timer->interval, timer->flags);
    }
    dbug_info("--------------timer--end-------------");
}
#else
#define display_timer()
#endif

static void insert_timer_sorted(net_timer_t* insert)
{
    nlist_node_t* node;
    nlist_for_each(node, &timer_list)
    {
        net_timer_t* current = nlist_entry(node, net_timer_t, node);
        if (insert->expire > current->expire)
        {
            insert->expire -= current->expire;
            continue;
        }
        if (insert->expire == current->expire)
        {
            insert->expire = 0;
        }
        else
        {
            current->expire -= insert->expire;
            nlist_node_t* prev = nlist_node_prev(&current->node);
            if (prev)
            {
                nlist_insert_after(&timer_list, prev, &insert->node);
            }
            else
            {
                nlist_insert_first(&timer_list, &insert->node);
            }
            return;
        }
        nlist_insert_after(&timer_list, node, &insert->node);
        return;
    }
    nlist_insert_last(&timer_list, &insert->node);
}

net_err_t net_timer_add(net_timer_t* timer,
                        const char* name,
                        const timer_proc_t proc,
                        void* arg,
                        const uint32_t ms,
                        const int flags
)
{
    plat_strncpy(timer->name, name, TIMER_NAME_LEN);
    timer->name[TIMER_NAME_LEN - 1] = '\0';
    timer->interval = ms;
    timer->expire = ms;
    timer->flags = flags;
    timer->proc = proc;
    timer->arg = arg;
    insert_timer_sorted(timer);
    display_timer();
    return NET_ERR_OK;
}

net_err_t net_timer_remove(const net_timer_t* timer)
{
    nlist_node_t* node = nlist_first(&timer_list);
    while (node)
    {
        net_timer_t* data = nlist_entry(node, net_timer_t, node);
        if (data == timer)
        {
            nlist_node_t* next = nlist_node_next(node);

            // 从链表中删除
            nlist_remove(&timer_list, node);

            // 是否存在下一个节点
            if (next)
            {
                net_timer_t* nextData = nlist_entry(next, net_timer_t, node);
                nextData->expire += timer->expire;
            }
            display_timer();
            return NET_ERR_OK;
        }
        node = node->next;
    }
    display_timer();
    return NET_ERR_INVALID_PARAM;
}
