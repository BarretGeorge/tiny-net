#include "net_api.h"
#include <ctype.h>

char* x_inet_ntoa(struct in_addr in)
{
    return 0;
}

uint32_t x_inet_addr(const char* cp)
{
    if (cp == NULL)
    {
        return 0;
    }
    uint32_t addr = 0;
    uint32_t val = 0;
    int dots = 0;
    while (*cp)
    {
        if (isdigit((unsigned char)*cp))
        {
            val = val * 10 + (*cp - '0');
            if (val > 255)
                return INADDR_NONE;
        }
        else if (*cp == '.')
        {
            if (++dots > 3)
                return INADDR_NONE;

            addr = (addr << 8) | val;
            val = 0;
        }
        else
        {
            return INADDR_NONE;
        }
        cp++;
    }
    if (dots != 3)
    {
        return INADDR_NONE;
    }

    addr = (addr << 8) | val;
    return addr;
}

int x_inet_pton(int family, const char* src, void* dst)
{
    return 0;
}

const char* x_inet_ntop(int family, const void* src, char* dst, size_t size)
{
    return 0;
}
