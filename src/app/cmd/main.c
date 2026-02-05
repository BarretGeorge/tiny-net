#include <stdbool.h>
#include "dbug.h"
#include "common.h"
#include "sys_plat.h"

int main(void)
{
    dbug_module_enable_all();

    tiny_net_init();

    while (true)
    {
        sys_sleep(100);
    }
}