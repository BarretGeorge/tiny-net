#include <stdio.h>
#include "sys_plat.h"
#include "net.h"
#include "netif_pcap.h"
#include "dbug.h"
#include "mblock.h"
#include "pktbuf.h"

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

void pktbuf_test()
{
    pktbuf_init();
    pktbuf_t* pktbuf = pktbuf_alloc(2000);
    // plat_printf("alloc pktbuf=%p,total_size=%d\n", pktbuf, pktbuf->total_size);
    // pktbuf_free(pktbuf);
    // plat_printf("free pktbuf=%p\n", pktbuf);

    for (int i = 0; i < 16; ++i)
    {
        pktbuf_add_header(pktbuf, 33, true);
    }
    plat_printf("添加完成\n");

    for (int i = 0; i < 16; ++i)
    {
        pktbuf_remove_header(pktbuf, 33);
    }
    plat_printf("移除完成\n");


    for (int i = 0; i < 16; ++i)
    {
        pktbuf_add_header(pktbuf, 33, false);
    }
    plat_printf("添加完成\n");

    for (int i = 0; i < 16; ++i)
    {
        pktbuf_remove_header(pktbuf, 33);
    }
    plat_printf("移除完成\n");

    pktbuf_free(pktbuf);

    pktbuf = pktbuf_alloc(8);
    pktbuf_resize(pktbuf, 32);
    pktbuf_resize(pktbuf, 200);
    pktbuf_resize(pktbuf, 4000);
    pktbuf_resize(pktbuf, 1921);
    pktbuf_resize(pktbuf, 32);
    pktbuf_resize(pktbuf, 8);
    pktbuf_resize(pktbuf, 0);
    pktbuf_free(pktbuf);


    pktbuf_t* pktbuf1 = pktbuf_alloc(432);
    pktbuf_t* pktbuf2 = pktbuf_alloc(256);
    join_pktbuf(pktbuf1, pktbuf2);
    pktbuf_free(pktbuf1);

    pktbuf_t* pktbuf3 = pktbuf_alloc(32);
    join_pktbuf(pktbuf3, pktbuf_alloc(4));
    join_pktbuf(pktbuf3, pktbuf_alloc(16));
    join_pktbuf(pktbuf3, pktbuf_alloc(54));
    join_pktbuf(pktbuf3, pktbuf_alloc(32));
    join_pktbuf(pktbuf3, pktbuf_alloc(38));
    pktbuf_set_cont(pktbuf3, 44);
    pktbuf_set_cont(pktbuf3, 60);
    pktbuf_set_cont(pktbuf3, 44);
    pktbuf_set_cont(pktbuf3, 128);
    pktbuf_set_cont(pktbuf3, 135);
    pktbuf_free(pktbuf3);


    pktbuf_t* pktbuf4 = pktbuf_alloc(32);
    join_pktbuf(pktbuf4, pktbuf_alloc(4));
    join_pktbuf(pktbuf4, pktbuf_alloc(16));
    join_pktbuf(pktbuf4, pktbuf_alloc(54));
    join_pktbuf(pktbuf4, pktbuf_alloc(32));
    join_pktbuf(pktbuf4, pktbuf_alloc(38));
    pktbuf_reset_access(pktbuf4);

    static uint16_t temp[1000];
    for (int i = 0; i < 1000; ++i)
    {
        temp[i] = i;
    }
    pktbuf_write(pktbuf4, (uint8_t*)temp, pktbuf_total(pktbuf4));

    plat_memset(temp, 0, sizeof(temp));
    pktbuf_seek(pktbuf4, 16);
    pktbuf_read(pktbuf4, (uint8_t*)temp, 100);
    for (int i = 0; i < 100; ++i)
    {
        plat_printf("temp[%d]=%d\n", i, temp[i]);
    }
    pktbuf_free(pktbuf4);

    pktbuf_t* dist = pktbuf_alloc(1024);
    pktbuf_t* src = pktbuf_alloc(1024);

    for (int i = 0; i < 512; ++i)
    {
        temp[i] = i;
    }
    pktbuf_write(src, (uint8_t*)temp, 512);

    pktbuf_seek(dist, 600);
    pktbuf_seek(src, 0);
    pktbuf_copy(dist, src, 100);

    // 验证复制结果
    plat_memset(temp, 0, sizeof(temp));
    pktbuf_seek(dist, 590);
    pktbuf_read(dist, (uint8_t*)temp, 100);
    for (int i = 0; i < 100; ++i)
    {
        plat_printf("copy temp[%d]=%d\n", i, temp[i]);
    }

    pktbuf_seek(src, 0);
    pktbuf_fill(src, 66, 1024);
    pktbuf_seek(src, 0);

    plat_memset(temp, 0, sizeof(temp));
    pktbuf_read(src, (uint8_t*)temp, 1024);


    pktbuf_free(dist);
    pktbuf_free(src);
}

void print_node_callback(void* arg)
{
    plat_printf("node data=%s\n", (char*)arg);
}

int main(void)
{
    // mblock_test();

    pktbuf_test();

    return 0;

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
