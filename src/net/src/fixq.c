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

net_err_t fixq_send(fixq_t* q, void* msg, const int32_t timeout_ms)
{
    if (q == NULL)
    {
        return NET_ERR_SYS;
    }
    nlocker_lock(&q->locker);
    if (timeout_ms < 0 && q->cnt == q->size)
    {
        // 队列已满，且不等待
        nlocker_unlock(&q->locker);
        return NET_ERR_MEM;
    }
    nlocker_unlock(&q->locker);

    if (sys_sem_wait(q->send_sem, (uint32_t)timeout_ms) < 0)
    {
        return NET_ERR_MEM; // 超时或出错
    }

    nlocker_lock(&q->locker);
    q->buf[q->in++] = msg;
    if (q->in >= q->size)
    {
        q->in = 0;
    }
    q->cnt++;
    nlocker_unlock(&q->locker);
    sys_sem_notify(q->recv_sem);
    return NET_ERR_OK;
}

void* fixq_recv(fixq_t* q, const int32_t timeout_ms)
{
    if (q == NULL)
    {
        return NULL;
    }
    nlocker_lock(&q->locker);
    if (timeout_ms < 0 && q->cnt == 0)
    {
        // 队列为空，且不等待
        nlocker_unlock(&q->locker);
        return NULL;
    }
    nlocker_unlock(&q->locker);
    if (sys_sem_wait(q->recv_sem, (uint32_t)timeout_ms) < 0)
    {
        return NULL; // 超时或出错
    }
    nlocker_lock(&q->locker);
    void* msg = q->buf[q->out++];
    if (q->out >= q->size)
    {
        q->out = 0;
    }
    q->cnt--;
    nlocker_unlock(&q->locker);

    sys_sem_notify(q->send_sem);
    return msg;
}

void fixq_destroy(fixq_t* q)
{
    q->size = q->in = q->out = q->cnt = 0;
    q->buf = NULL;
    sys_sem_free(q->recv_sem);
    sys_sem_free(q->send_sem);
    nlocker_destroy(&q->locker);
}

int fixq_count(const fixq_t* q)
{
    nlocker_lock(&q->locker);
    const int cnt = q->cnt;
    nlocker_unlock(&q->locker);
    return cnt;
}