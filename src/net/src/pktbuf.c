#include "pktbuf.h"
#include "dbug.h"
#include "mblock.h"
#include "nlocker.h"

static nlocker_t locker;

static pktblk_t block_buffer[PKTBUF_BLK_COUNT];

static mblock_t block_list;

static pktbuf_t pktbuf_buffer[PKTBUF_BUF_COUNT];

static mblock_t pktbuf_list;

net_err_t pktbuf_init()
{
    dbug_info("pktbuf init...");

    // 初始化锁
    nlocker_init(&locker, NLOCKER_TYPE_THREAD);

    // 初始化 pktblk_t 内存块管理器
    mblock_init(&block_list, block_buffer, PKTBUF_BLK_COUNT, sizeof(pktblk_t), NLOCKER_TYPE_THREAD);

    // 初始化 pktbuf 内存块管理器
    mblock_init(&pktbuf_list, pktbuf_buffer, PKTBUF_BUF_COUNT, sizeof(pktbuf_t), NLOCKER_TYPE_THREAD);

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
        nlist_node_init(&blk->node, blk, NULL, NULL);
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

static pktblk_t* pktblock_get_next(const pktblk_t* block)
{
    if (block == NULL || block->node.next == NULL)
    {
        return NULL;
    }
    return (pktblk_t*)block->node.next->data;
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
                nlist_inset_head(&buf->blk_list, &block->node);
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
            nlist_inset_tail(&buf->blk_list, &block->node);
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
    nlist_node_init(&buf->node, NULL, NULL, NULL);
    if (size)
    {
        pktblk_t* block = pktblock_alloc_list(size, false);
        if (block == NULL)
        {
            dbug_error("pktblock alloc list failed");
            mblock_free(&pktbuf_list, buf);
            return NULL;
        }

        // todo 根据 size 大小决定头插法还是尾插法
        if (size > 1000)
        {
            pktbuf_insert_blk_list(buf, block, false);
        }
        else
        {
            pktbuf_insert_blk_list(buf, block, true);
        }
    }
    return buf;
}

void pktbuf_free(pktbuf_t* pktbuf)
{
}
