#include "common.h"
#include "sys_plat.h"
#include <stdbool.h>

int main(void)
{
    tiny_net_init();

    while (true)
    {
        sys_sleep(100);
    }
}
