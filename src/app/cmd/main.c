#include <stdio.h>
#include "sys_plat.h"
#include "net.h"
#include "netif_pcap.h"
#include "dbug.h"
#include "mblock.h"

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

sys_sem_t sem;

void threadConsume(void* arg)
{
    while (1)
    {
        sys_sem_wait(sem, 0);
        sys_sleep(1000);
        plat_printf("消费者消费一次 arg=%s\n", (char*)arg);
    }
}

void threadProduce(void* arg)
{
    while (1)
    {
        sys_sleep(1500);
        plat_printf("生产者生产一份完成 arg=%s\n", (char*)arg);
        sys_sem_notify(sem);
    }
}

net_err_t netdev_init(void)
{
    netif_pcap_open();
    return NET_ERR_OK;
}

void mblock_test()
{
    mblock_t mblock;
    static uint8_t buff[256][10];
    mblock_init(&mblock, buff, 256, 10, NLOCKER_TYPE_THREAD);

    void* temp[10];
    for (int i = 0; i < 10; i++)
    {
        temp[i] = mblock_alloc(&mblock, 100);
        plat_printf("alloc %d block addr=%p,free_cnt=%d \n", i, temp[i], mblock_free_cnt(&mblock));
    }
    for (int i = 0; i < 10; i++)
    {
        mblock_free(&mblock, temp[i]);
        plat_printf("free %d block addr=%p,free_cnt=%d \n", i, temp[i], mblock_free_cnt(&mblock));
    }
}

int main(void)
{
    net_init();

    net_start();

    netdev_init();

    while (1)
    {
        sys_sleep(100);
    }

    sem = sys_sem_create(0);
    // 创建线程
    sys_thread_create(threadConsume, "ttx");
    sys_thread_create(threadProduce, "ttx");

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
