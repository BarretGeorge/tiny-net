#include "exmsg.h"
#include "sys_plat.h"

net_err_t exmsg_init()
{
    return NET_ERR_OK;
}

static void work_thread(void* arg)
{
    while (1)
    {
        // 处理扩展消息
        sys_sleep(1000);
    }
}

net_err_t exmsg_start()
{
    const sys_thread_t thread = sys_thread_create(work_thread, NULL);
    if (thread == SYS_THREAD_INVALID)
    {
        return NET_ERR_SYS;
    }
    return NET_ERR_OK;
}
