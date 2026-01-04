#ifndef TINY_NET_IPADDR_H
#define TINY_NET_IPADDR_H

#include "net_err.h"
#include "sys.h"
#include <stdint.h>

#define IPV4_ADDR_LEN 4

typedef struct ipaddr_t
{
    enum
    {
        IPADDR_TYPE_V4,
        IPADDR_TYPE_V6
    } type;

    union
    {
        uint8_t q_addr[IPV4_ADDR_LEN];
        uint32_t a_addr;
    };
} ipaddr_t;

#define dbug_dump_hwaddr(addr, len)          \
    do {                                \
        int _i;                         \
        for (_i = 0; _i < (len); _i++)  \
        {                               \
            plat_printf("%02X", (addr)[_i]); \
            if (_i < (len) - 1)         \
                plat_printf(":");        \
        }                               \
        plat_printf("\n");               \
    } while(0)

#define dbug_dump_ipaddr(ip)               \
    do {                                \
        if ((ip)->type == IPADDR_TYPE_V4) \
        {                               \
            plat_printf("%d.%d.%d.%d\n", \
                (ip)->q_addr[0],        \
                (ip)->q_addr[1],        \
                (ip)->q_addr[2],        \
                (ip)->q_addr[3]);       \
        }                               \
        else                            \
        {                               \
            plat_printf("IPv6 not supported\n"); \
        }                               \
    } while(0)

void ipaddr_set_any(ipaddr_t* ip);

net_err_t ipaddr4_form_str(ipaddr_t* ip, const char* str);

const ipaddr_t* get_addr_any();

void ipaddr_copy(ipaddr_t* dest, const ipaddr_t* src);

#endif //TINY_NET_IPADDR_H
