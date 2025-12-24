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
    dbug_info("exmsg work_thread started");
    while (1)
    {
        // 接收消息，阻塞等待
        exmsg_t* msg = fixq_recv(&msg_queue, -1);
        if (msg == NULL)
        {
            continue;
        }
        // 打印消息id 模拟处理消息
        dbug_info("exmsg work_thread: received msg type=%d, id=%d", msg->type, msg->id);

        // 释放消息内存块
        mblock_free(&msg_mblock, msg);
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

net_err_t exmsg_netif_in()
{
    exmsg_t* msg = mblock_alloc(&msg_mblock, -1);
    if (msg == NULL)
    {
        dbug_warn("no free exmsg block");
        return NET_ERR_MEM;
    }
    static int counter = 0;
    msg->type = NET_EXMSG_TYPE_NETIF_IN;
    msg->id = ++counter;
    if (fixq_send(&msg_queue, msg, -1) != NET_ERR_OK)
    {
        dbug_error("fixq full");
        mblock_free(&msg_mblock, msg);
        return NET_ERR_MEM;
    }
    dbug_info("发送 exmsg netif_in 消息, id=%d", msg->id);
    return NET_ERR_OK;
}
