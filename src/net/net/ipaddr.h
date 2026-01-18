#ifndef TINY_NET_IPADDR_H
#define TINY_NET_IPADDR_H

#include <stdint.h>
#include "net_err.h"
#include "sys.h"

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
        uint8_t a_addr[IPV4_ADDR_LEN];
        uint32_t q_addr;
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
                (ip)->a_addr[0],        \
                (ip)->a_addr[1],        \
                (ip)->a_addr[2],        \
                (ip)->a_addr[3]);       \
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

void ipaddr_to_buf(const ipaddr_t* ip, uint8_t* buf);

#endif //TINY_NET_IPADDR_H
