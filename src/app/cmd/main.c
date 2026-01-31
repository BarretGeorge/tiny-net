#include "common.h"
#include "sys_plat.h"
#include <stdbool.h>

#include "dbug.h"
#include "exmsg.h"

net_err_t test_func(const func_msg_t* msg)
{
    dbug_info("test func exec in thread %s", msg->arg);
    return NET_ERR_OK;
}

int main(void)
{
    tiny_net_init();

    char name[16] = "12345";

    exmsg_func_exec(test_func, name);

    while (true)
    {
        sys_sleep(100);
    }
}