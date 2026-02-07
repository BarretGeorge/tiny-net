#include "tool.h"

uint16_t checksum16(const void* data, uint16_t size, const uint32_t pre_sum, bool complement)
{
    uint16_t* curr_buf = (uint16_t*)data;
    uint32_t checksum = pre_sum;

    while (size > 1)
    {
        checksum += *curr_buf++;
        size -= 2;
    }

    if (size > 0)
    {
        checksum += *(uint8_t*)curr_buf;
    }

    uint16_t high;
    while ((high = checksum >> 16) != 0)
    {
        checksum = high + (checksum & 0xffff);
    }

    return complement ? (uint16_t)~checksum : (uint16_t)checksum;
}

uint16_t checksum16_pseudo(pktbuf_t* buf, const ipaddr_t* src_ip, const ipaddr_t* dst_ip, const uint8_t protocol)
{
    uint8_t pseudo_hdr[12];

    pseudo_hdr[0] = src_ip->a_addr[0];
    pseudo_hdr[1] = src_ip->a_addr[1];
    pseudo_hdr[2] = src_ip->a_addr[2];
    pseudo_hdr[3] = src_ip->a_addr[3];

    pseudo_hdr[4] = dst_ip->a_addr[0];
    pseudo_hdr[5] = dst_ip->a_addr[1];
    pseudo_hdr[6] = dst_ip->a_addr[2];
    pseudo_hdr[7] = dst_ip->a_addr[3];

    pseudo_hdr[8] = 0;
    pseudo_hdr[9] = protocol;

    uint16_t total = (uint16_t)buf->total_size;
    pseudo_hdr[10] = (uint8_t)(total >> 8);
    pseudo_hdr[11] = (uint8_t)(total & 0xFF);

    uint16_t pre_sum = checksum16(pseudo_hdr, sizeof(pseudo_hdr), 0, false);
    pktbuf_reset_access(buf);
    return pktbuf_checksum16(buf, (int)buf->total_size, pre_sum, true);
}
