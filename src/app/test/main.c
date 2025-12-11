#include <stdio.h>
#include "sys_plat.h"

void sendPacket(pcap_t* pcap, const uint8_t* data, const int len)
{
    if (pcap_inject(pcap, data, len) < 0)
    {
        plat_printf("pcap_inject failed,err %s\n", pcap_geterr(pcap));
        exit(1);
    }
}

u_int recvPacket(pcap_t* pcap, uint8_t* buffer, const int len)
{
    struct pcap_pkthdr* header;
    const uint8_t* data;
    const int res = pcap_next_ex(pcap, &header, &data);
    if (res == 0)
    {
        return 0;
    }
    if (res < 0)
    {
        plat_printf("pcap_next_ex failed, code: %d, err: %s\n", res, pcap_geterr(pcap));
        return 0;
    }
    const uint length = header->len > len ? len : header->len;
    plat_memcpy(buffer, data, length);
    return length;
}

int main(void)
{
    // 网卡的硬件地址
    static const uint8_t netdev0_hwaddr[] = {0x00, 0x50, 0x56, 0xc0, 0x00, 0x11};

    // 第一个参数为要打开的网卡ip地址，第二个参数为网卡的硬件地址
    pcap_t* pcap = pcap_device_open("192.168.100.94", netdev0_hwaddr);
    if (!pcap)
    {
        plat_printf("pcap_device_open failed,err:%s\n", pcap_geterr(pcap));
        return -1;
    }
    while (1)
    {
        static uint8_t buffer[1024];
        static int counter = 0;

        for (int i = 0; i < sizeof(buffer); i++)
        {
            buffer[i] = i;
        }
        const uint len = recvPacket(pcap, buffer, sizeof(buffer));
        if (len == 0)
        {
            continue;
        }
        if (len < 0)
        {
            plat_printf("recvPacket failed\n");
            continue;
        }
        plat_printf("recv packet len=%d,data=%s\n", len, (char*)buffer);
        buffer[0] ^= 0xff; // 修改数据，模拟发送不同的数据包
        sendPacket(pcap, buffer, sizeof(buffer));
        plat_printf("send packet %d\n", counter++);
        sys_sleep(500);
    }
    return 0;
}
