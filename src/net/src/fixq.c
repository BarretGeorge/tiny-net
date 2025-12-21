#include "fixq.h"


net_err_t fixq_init(fixq_t* q, void** buf, const int size, const nlocker_type_t share_type)
{
    net_err_t err = NET_ERR_OK;

    // 初始化锁
    if (nlocker_init(&q->locker, share_type) != NET_ERR_OK)
    {
        err = NET_ERR_SYS;
        goto fail;
    }

    // 创建信号量
    q->recv_sem = sys_sem_create(0);
    if (q->recv_sem == SYS_SEM_INVALID)
    {
        err = NET_ERR_SYS;
        goto fail;
    }

    q->send_sem = sys_sem_create(size);
    if (q->send_sem == SYS_SEM_INVALID)
    {
        err = NET_ERR_SYS;
        goto fail;
    }

    q->size = size;
    q->in = 0;
    q->out = 0;
    q->cnt = 0;
    q->buf = buf;

fail:
    if (q->recv_sem != SYS_SEM_INVALID)
    {
        sys_sem_free(q->recv_sem);
    }
    if (q->send_sem != SYS_SEM_INVALID)
    {
        sys_sem_free(q->send_sem);
    }
    nlocker_destroy(&q->locker);
    return err;
}


void fixq_destroy(fixq_t* q)
{
    q->size = q->in = q->out = q->cnt = 0;
    q->buf = NULL;
    sys_sem_free(q->recv_sem);
    sys_sem_free(q->send_sem);
    nlocker_destroy(&q->locker);
}
