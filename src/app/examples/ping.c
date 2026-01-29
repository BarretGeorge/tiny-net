#include "ping/ping.h"

#include "common.h"

int main()
{
    tiny_net_init();

    ping_t ping;
    ping_run(&ping, "192.168.100.108", 4, 1000, 1000, 500);

    while (true)
    {
        sys_sleep(100);
    }
}
