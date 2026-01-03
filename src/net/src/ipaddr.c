#include "ipaddr.h"

void ipaddr_set_any(ipaddr_t* ip)
{
    if (!ip)
    {
        return;
    }
    ip->type = IPADDR_TYPE_V4;
    ip->a_addr = 0;
}
