#include "ipaddr.h"
#include "sys.h"
#include "dbug.h"
#include <ctype.h>

void ipaddr_set_any(ipaddr_t* ip)
{
    if (!ip)
    {
        return;
    }
    ip->type = IPADDR_TYPE_V4;
    ip->q_addr = 0;
}


net_err_t ipaddr4_form_str(ipaddr_t* ip, const char* str)
{
    if (!ip || !str)
    {
        return NET_ERR_INVALID_PARAM;
    }

    uint8_t* p = (uint8_t*)&ip->a_addr;
    int val = 0;
    int index = 0;
    int char_cnt = 0;
    char c;

    ip->type = IPADDR_TYPE_V4;
    ip->q_addr = 0; // 清零

    while ((c = *str++) != '\0')
    {
        if (isdigit((unsigned char)c))
        {
            val = val * 10 + (c - '0');
            // 检查溢出：IPv4 单段最大 255
            if (val > 255)
            {
                return NET_ERR_INVALID_PARAM;
            }
            char_cnt++;
        }
        else if (c == '.')
        {
            // 遇到分隔符
            if (index >= 3) return NET_ERR_INVALID_PARAM; // 段数过多
            if (char_cnt == 0) return NET_ERR_INVALID_PARAM; // 空段 (例如 ..)
            p[index++] = (uint8_t)val;
            val = 0;
            char_cnt = 0;
        }
        else
        {
            // 遇到非数字且非点的字符 (例如 192.168.1.a)
            return NET_ERR_INVALID_PARAM;
        }
    }

    // 处理最后一段
    if (index != 3 || char_cnt == 0)
    {
        return NET_ERR_INVALID_PARAM;
    }
    p[index] = (uint8_t)val;
    return NET_ERR_OK;
}

const ipaddr_t* get_addr_any()
{
    static ipaddr_t addr_any;
    ipaddr_set_any(&addr_any);
    return &addr_any;
}

void ipaddr_copy(ipaddr_t* dest, const ipaddr_t* src)
{
    plat_memcpy(dest, src, sizeof(ipaddr_t));
}

void ipaddr_to_buf(const ipaddr_t* ip, uint8_t* buf)
{
    if (ip->type == IPADDR_TYPE_V4)
    {
        *(uint32_t*)buf = ip->q_addr;
    }
    else
    {
        dbug_error("IPv6 not supported");
    }
}

void ipaddr_from_buf(ipaddr_t* ip, const uint8_t* buf)
{
    ip->type = IPADDR_TYPE_V4;
    ip->q_addr = *(uint32_t*)buf;
}

int ipaddr_is_equal(const ipaddr_t* ip1, const ipaddr_t* ip2)
{
    if (ip1->type != ip2->type)
    {
        return 0;
    }
    if (ip1->type == IPADDR_TYPE_V4)
    {
        return ip1->q_addr == ip2->q_addr;
    }
    dbug_error("IPv6 not supported");
    return 0;
}

bool is_local_broadcast_ip(const ipaddr_t* ip)
{
    if (ip->type != IPADDR_TYPE_V4)
    {
        return false;
    }
    return ip->q_addr == 0xFFFFFFFF;
}

static ipaddr_t ipaddr_get_host(const ipaddr_t* netmask, const ipaddr_t* ip)
{
    ipaddr_t host;
    host.type = IPADDR_TYPE_V4;
    host.q_addr = ip->q_addr & (~netmask->q_addr);
    return host;
}

bool is_directed_broadcast_ip(const ipaddr_t* netmask, const ipaddr_t* ip)
{
    if (ip->type != IPADDR_TYPE_V4 || netmask->type != IPADDR_TYPE_V4)
    {
        return false;
    }
    ipaddr_t host = ipaddr_get_host(netmask, ip);
    ipaddr_t max_host;
    max_host.type = IPADDR_TYPE_V4;
    max_host.q_addr = ~netmask->q_addr;
    return host.q_addr == max_host.q_addr;

}