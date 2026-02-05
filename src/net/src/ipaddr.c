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

ipaddr_t* ipaddr_get_any()
{
    static ipaddr_t addr_any;
    ipaddr_set_any(&addr_any);
    return &addr_any;
}

bool ipaddr_is_any(const ipaddr_t* ip)
{
    if (!ip)
    {
        return false;
    }
    if (ip->type != IPADDR_TYPE_V4)
    {
        return false;
    }
    return ip->q_addr == 0;
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

void ipaddr_to_buf(const ipaddr_t* src, uint8_t* buf)
{
    buf[0] = src->a_addr[0];
    buf[1] = src->a_addr[1];
    buf[2] = src->a_addr[2];
    buf[3] = src->a_addr[3];
}

void ipaddr_to_str(const ipaddr_t* ip, char* buf, const int buf_len)
{
    if (ip->type != IPADDR_TYPE_V4 || buf_len < 16)
    {
        if (buf_len > 0)
        {
            buf[0] = '\0';
        }
        return;
    }
    plat_sprintf(buf, "%u.%u.%u.%u",
                 ip->a_addr[0],
                 ip->a_addr[1],
                 ip->a_addr[2],
                 ip->a_addr[3]);
}

void ipaddr_from_buf(ipaddr_t* dest, const uint8_t* buf)
{
    dest->a_addr[0] = buf[0];
    dest->a_addr[1] = buf[1];
    dest->a_addr[2] = buf[2];
    dest->a_addr[3] = buf[3];
}

int ipaddr_is_equal(const ipaddr_t* ip1, const ipaddr_t* ip2)
{
    return ip1->q_addr == ip2->q_addr;
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

ipaddr_t ipaddr_get_network(const ipaddr_t* netmask, const ipaddr_t* ip)
{
    ipaddr_t network;
    network.type = IPADDR_TYPE_V4;
    network.q_addr = ip->q_addr & netmask->q_addr;
    return network;
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

bool ipaddr_is_match(const ipaddr_t* dest_ip, const ipaddr_t* src_ip, const ipaddr_t* netmask)
{
    if (is_local_broadcast_ip(dest_ip))
    {
        return true;
    }

    ipaddr_t dest_net = ipaddr_get_network(netmask, dest_ip);
    ipaddr_t src_net = ipaddr_get_network(netmask, src_ip);

    if (is_directed_broadcast_ip(netmask, dest_ip) && ipaddr_is_equal(&dest_net, &src_net))
    {
        return true;
    }

    if (ipaddr_is_equal(&dest_net, &src_net))
    {
        return true;
    }
    return false;
}

int ipaddr_1_count(const ipaddr_t* ip)
{
    int count = 0;
    uint32_t val = ip->q_addr;
    while (val)
    {
        count += (int)val & 1;
        val >>= 1;
    }
    return count;
}
