#include "timer.h"
#include "dbug.h"

static nlist_t timer_list;

net_err_t net_timer_init()
{
    dbug_info("Initializing timer module...");
    nlist_init(&timer_list);
    return NET_ERR_OK;
}

net_err_t net_timer_add(net_timer_t* timer,
                        const char* name,
                        timer_proc_t proc,
                        void* arg,
                        uint32_t ms,
                        int flags
)
{
    return NET_ERR_OK;
}
