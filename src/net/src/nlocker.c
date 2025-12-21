#include "nlocker.h"

net_err_t nlocker_init(nlocker_t* locker, const nlocker_type_t type)
{
    if (locker == NULL)
    {
        return NET_ERR_SYS;
    }
    locker->type = type;
    if (locker->type == NLOCKER_TYPE_THREAD)
    {
        locker->mutex = sys_mutex_create();
        if (locker->mutex == SYS_MUTEX_INVALID)
        {
            return NET_ERR_SYS;
        }
    }
    return NET_ERR_OK;
}

void nlocker_destroy(const nlocker_t* locker)
{
    if (locker == NULL)
    {
        return;
    }
    if (locker->type == NLOCKER_TYPE_THREAD)
    {
        sys_mutex_free(locker->mutex);
    }
}

void nlocker_lock(const nlocker_t* locker)
{
    if (locker == NULL)
    {
        return;
    }
    if (locker->type == NLOCKER_TYPE_THREAD)
    {
        sys_mutex_lock(locker->mutex);
    }
}

void nlocker_unlock(const nlocker_t* locker)
{
    if (locker == NULL)
    {
        return;
    }
    if (locker->type == NLOCKER_TYPE_THREAD)
    {
        sys_mutex_unlock(locker->mutex);
    }
}
