#ifndef TINY_NET_IPADDR_H
#define TINY_NET_IPADDR_H

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

void ipaddr_set_any(ipaddr_t* ip);

#endif //TINY_NET_IPADDR_H
