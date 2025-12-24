#include "pktbuf.h"

#include <stdbool.h>

#include "dbug.h"
#include "mblock.h"
#include "nlocker.h"

static nlocker_t locker;

static pktblk_t block_buffer[PKTBUF_BLK_COUNT];

static mblock_t block_list;

static pktbuf_t pktbuf_buffer[PKTBUF_BUF_COUNT];

static mblock_t pktbuf_list;

static long curr_blk_tail_free(const pktblk_t* curr)
{
    return curr->payload + PKTBUF_PAYLOAD_SIZE - (curr->data + curr->size);
}

// static void pktblock_free_list(pktblk_t* first)
// {
//     while (first)
//     {
//         pktblk_t* next = pktblock_get_next(first);
//         mblock_free(&block_list, first);
//         first = next;
//     }
// }

static pktblk_t* pktbuf_first_blk(const pktbuf_t* pktbuf)
{
    nlist_node_t* first = nlist_first(&pktbuf->blk_list);
    return nlist_entry(first, pktblk_t, node);
}

#if DBG_DISPLAY_ENABLE(DBG_BUG)
static void display_check_buf(const pktbuf_t* pktbuf)
{
    if (!pktbuf)
    {
        dbug_error("pktbuf is NULL");
        return;
    }
    dbug_info("pktbuf total_size=%d", pktbuf->total_size);
    uint32_t total_size = 0;
    const pktblk_t* curr = NULL;
    int idx = 0;

    for (curr = pktbuf_first_blk(pktbuf); curr != NULL; curr = pktblock_get_next(curr))
    {
        plat_printf("idx:%d ,", idx++);

        if ((curr->data < curr->payload) || (curr->data > (curr->payload + PKTBUF_PAYLOAD_SIZE)))
        {
            dbug_error("pktblk data ptr error,addr=%p", curr->data);
            break;
        }

        const long pre_size = curr->data - curr->payload;
        plat_printf("pre: %ld b,", pre_size);
        const uint32_t used_size = curr->size;
        plat_printf("used: %d b,", used_size);
        const long free_size = curr_blk_tail_free(curr);
        plat_printf("free: %ld b\n", free_size);

        const long total = pre_size + used_size + free_size;
        if (total != PKTBUF_PAYLOAD_SIZE)
        {
            dbug_error("pktblk size error,total=%ld", total);
        }
        total_size += curr->size;
    }
    if (total_size != pktbuf->total_size)
    {
        dbug_error("pktbuf total_size error,calc=%d,buf=%d", total_size, pktbuf->total_size);
    }
}
#else
#define display_pktbuf(buf)
#endif

net_err_t pktbuf_init()
{
    dbug_info("pktbuf init...");

    // 初始化锁
    nlocker_init(&locker, NLOCKER_TYPE_THREAD);

    // 初始化 pktblk_t 内存块管理器
    mblock_init(&block_list, block_buffer, sizeof(pktblk_t), PKTBUF_BLK_COUNT, NLOCKER_TYPE_THREAD);

    // 初始化 pktbuf 内存块管理器
    mblock_init(&pktbuf_list, pktbuf_buffer, sizeof(pktbuf_t),PKTBUF_BUF_COUNT, NLOCKER_TYPE_THREAD);

    dbug_info("pktbuf init ok");
    return NET_ERR_OK;
}

static pktblk_t* pktblock_alloc()
{
    nlocker_lock(&locker);
    pktblk_t* blk = mblock_alloc(&block_list, 0);
    nlocker_unlock(&locker);
    if (blk)
    {
        blk->size = 0;
        blk->data = 0;
        nlist_node_init(&blk->node);
    }
    return blk;
}

static pktblk_t* pktblock_alloc_list(int size, const bool is_head)
{
    pktblk_t* first_blk = NULL;
    pktblk_t* prev_blk = NULL;
    while (size)
    {
        pktblk_t* new_blk = pktblock_alloc();
        if (new_blk == NULL)
        {
            dbug_error("pktblock alloc(%d) failed", size);
            // todo free 已经分配的块
            return NULL;
        }

        const int curr_size = size > PKTBUF_PAYLOAD_SIZE ? PKTBUF_PAYLOAD_SIZE : size;
        new_blk->size = curr_size;

        if (is_head) // 是否头插法
        {
            new_blk->data = new_blk->payload + (PKTBUF_PAYLOAD_SIZE - curr_size);
            if (first_blk)
            {
                new_blk->node.next = &first_blk->node;
                first_blk->node.prev = &new_blk->node;
            }
            first_blk = new_blk;
        }
        else
        {
            new_blk->data = new_blk->payload;

            if (first_blk == NULL)
            {
                first_blk = new_blk;
            }

            if (prev_blk)
            {
                prev_blk->node.next = &new_blk->node;
                new_blk->node.prev = &prev_blk->node;
            }
            prev_blk = new_blk;
        }
        size -= curr_size;
    }
    return first_blk;
}

static void pktbuf_insert_blk_list(pktbuf_t* buf, pktblk_t* block, const bool is_head)
{
    if (is_head) // 头插法
    {
        pktblk_t* prev_blk = NULL;

        while (block)
        {
            pktblk_t* next_blk = pktblock_get_next(block);
            buf->total_size += block->size;
            if (prev_blk)
            {
                nlist_insert_after(&buf->blk_list, &prev_blk->node, &block->node);
            }
            else
            {
                nlist_insert_first(&buf->blk_list, &block->node);
            }
            prev_blk = block;
            block = next_blk;
        }
    }
    else // 尾插法
    {
        while (block)
        {
            pktblk_t* next_blk = pktblock_get_next(block);
            nlist_insert_last(&buf->blk_list, &block->node);
            buf->total_size += block->size;
            block = next_blk;
        }
    }
}

pktbuf_t* pktbuf_alloc(const int size)
{
    pktbuf_t* buf = mblock_alloc(&pktbuf_list, 0);
    if (buf == NULL)
    {
        dbug_error("pktbuf alloc failed");
        return NULL;
    }
    buf->total_size = 0;
    nlist_init(&buf->blk_list);
    nlist_node_init(&buf->node);

    bool is_head = true;
    if (size > 1000)
    {
        is_head = false;
    }

    if (size)
    {
        pktblk_t* block = pktblock_alloc_list(size, is_head);
        if (block == NULL)
        {
            dbug_error("pktblock alloc list failed");
            mblock_free(&pktbuf_list, buf);
            return NULL;
        }
        pktbuf_insert_blk_list(buf, block, is_head);
    }
    display_check_buf(buf);
    return buf;
}

void pktbuf_free(pktbuf_t* pktbuf)
{
    if (!pktbuf) return;

    nlocker_lock(&locker);

    // free 块链表
    // pktblock_free_list(pktbuf_first_blk(pktbuf));
    pktblk_t* curr = pktbuf_first_blk(pktbuf);
    while (curr)
    {
        pktblk_t* next = pktblock_get_next(curr);
        mblock_free(&block_list, curr);
        curr = next;
    }

    // free pktbuf
    mblock_free(&pktbuf_list, pktbuf);

    nlocker_unlock(&locker);
}
