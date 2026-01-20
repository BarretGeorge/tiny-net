#include "pktbuf.h"
#include "sys.h"

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

int main()
{
    pktbuf_test();
}