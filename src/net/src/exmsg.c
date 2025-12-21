#include "exmsg.h"
#include "net_cfg.h"
#include "dbug.h"
#include "sys_plat.h"
#include "fixq.h"
#include "mblock.h"

static void* msg_tbl[EXMSG_QUEUE_SIZE];

static fixq_t msg_queue;

static exmsg_t msg_buf[EXMSG_QUEUE_SIZE];

static mblock_t msg_mblock;

net_err_t exmsg_init()
{
    dbug_info("exmsg init...");

    net_err_t err = NET_ERR_OK;

    // 初始化消息内存块
    err = mblock_init(&msg_mblock, msg_buf, sizeof(exmsg_t), EXMSG_QUEUE_SIZE, NLOCKER_TYPE_THREAD);
    if (err != NET_ERR_OK)
    {
        dbug_error("exmsg mblock init failed, err=%d", err);
        goto fail;
    }

    // 初始化消息队列
    err = fixq_init(&msg_queue, msg_tbl, EXMSG_QUEUE_SIZE, NLOCKER_TYPE_THREAD);
    if (err != NET_ERR_OK)
    {
        dbug_error("exmsg queue init failed, err=%d", err);
        goto fail;
    }

    return NET_ERR_OK;

fail:
    mblock_destroy(&msg_mblock);
    fixq_destroy(&msg_queue);
    return err;
}

static void work_thread(void* arg)
{
    plat_printf("exmsg work_thread started\n");
    while (1)
    {
        // 处理扩展消息
        sys_sleep(100);
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
