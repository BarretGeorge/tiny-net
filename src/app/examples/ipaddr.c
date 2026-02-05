#include "ipaddr.h"

int main()
{
    ipaddr_t ip;
    ip.a_addr[0] = 192;
    ip.a_addr[1] = 168;
    ip.a_addr[2] = 1;
    ip.a_addr[3] = 100;
    ip.type = IPADDR_TYPE_V4;

    char str[16];
    ipaddr_to_str(&ip, str, sizeof(str));
    plat_printf("IP Address: %s\n", str);
    return 0;
}
