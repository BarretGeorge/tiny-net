#include "mblock.h"

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

int main()
{
    mblock_test();
}
