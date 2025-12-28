#include <stdbool.h>
#include "pktbuf.h"
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

static void pktblock_free(pktblk_t* block)
{
    if (block)
    {
        mblock_free(&block_list, block);
    }
}

static void pktblock_free_list(pktblk_t* first)
{
    while (first)
    {
        pktblk_t* next = pktblock_get_next(first);
        pktblock_free(first);
        first = next;
    }
}

static pktblk_t* pktbuf_last_blk(const pktbuf_t* pktbuf)
{
    nlist_node_t* last = nlist_last(&pktbuf->blk_list);
    return nlist_entry(last, pktblk_t, node);
}

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
        plat_printf("idx:%d,", idx++);

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

    /**
    添加包头 即为 发送数据 需要头插法
    移除包头 即为 接收数据 需要尾插法
     */

    // todo 写死头插法
    bool is_head = true;

    if (size)
    {
        pktblk_t* block = pktblock_alloc_list(size, is_head);
        if (block == NULL)
        {
            dbug_error("pktblock alloc list failed");
            pktblock_free(block);
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

net_err_t pktbuf_add_header(pktbuf_t* pktbuf, int size, const bool is_cont)
{
    if (!pktbuf || size <= 0)
    {
        return NET_ERR_INVALID_PARAM;
    }

    pktblk_t* block = pktbuf_first_blk(pktbuf);

    // 获取剩余空间
    const int remain_size = (int)(block->data - block->payload);
    if (size <= remain_size) // 当前块有足够空间 直接分配
    {
        block->size += size;
        block->data -= size;
        pktbuf->total_size += size;
        display_check_buf(pktbuf);
        return NET_ERR_OK;
    }

    pktblk_t* new_blk = NULL;
    if (is_cont) // 需要连续空间
    {
        if (size > PKTBUF_PAYLOAD_SIZE)
        {
            dbug_error("pktbuf add header size too large for cont,size=%d", size);
            return NET_ERR_INVALID_PARAM;
        }
        // 分配新块
        new_blk = pktblock_alloc_list(size, true);
    }
    else // 非连续空间，分配多块
    {
        block->data = block->payload;
        block->size += remain_size;
        pktbuf->total_size += remain_size;
        size -= remain_size;

        // 分配新块
        new_blk = pktblock_alloc_list(size, true);
    }

    if (new_blk == NULL)
    {
        dbug_error("pktblock alloc failed");
        return NET_ERR_MEM;
    }

    pktbuf_insert_blk_list(pktbuf, new_blk, true);
    display_check_buf(pktbuf);
    return NET_ERR_OK;
}

net_err_t pktbuf_remove_header(pktbuf_t* pktbuf, int size)
{
    if (!pktbuf || size <= 0)
    {
        return NET_ERR_INVALID_PARAM;
    }
    pktblk_t* block = pktbuf_first_blk(pktbuf);
    if (!block)
    {
        dbug_error("pktbuf remove header failed,buf is empty");
        return NET_ERR_SYS;
    }

    while (size > 0 && block != NULL)
    {
        pktblk_t* next = pktblock_get_next(block);
        if (size < block->size)
        {
            block->data += size;
            block->size -= size;
            pktbuf->total_size -= size;
            break;
        }
        const int curr_size = (int)block->size;
        nlist_remove_first(&pktbuf->blk_list);
        pktblock_free(block);
        pktbuf->total_size -= curr_size;
        size -= curr_size;
        block = next;
    }
    display_check_buf(pktbuf);
    return NET_ERR_OK;
}

net_err_t pktbuf_resize(pktbuf_t* pktbuf, const int new_size)
{
    if (!pktbuf || new_size < 0)
    {
        return NET_ERR_INVALID_PARAM;
    }
    if (new_size == (int)pktbuf->total_size)
    {
        return NET_ERR_OK;
    }
    if (pktbuf->total_size == 0)
    {
        pktblk_t* blk = pktblock_alloc_list(new_size, false);
        if (blk == NULL)
        {
            dbug_error("pktblock alloc list failed");
            return NET_ERR_MEM;
        }
        pktbuf_insert_blk_list(pktbuf, blk, false);
        pktbuf->total_size = new_size;
        return NET_ERR_OK;
    }
    if (new_size == 0)
    {
        pktblock_free_list(pktbuf_first_blk(pktbuf));
        nlist_init(&pktbuf->blk_list);
        pktbuf->total_size = 0;
        return NET_ERR_OK;
    }
    if (pktbuf->total_size < (uint32_t)new_size) // 扩展
    {
        pktblk_t* last_blk = pktbuf_last_blk(pktbuf);
        const int inc_size = new_size - (int)pktbuf->total_size;
        const int remain_size = (int)curr_blk_tail_free(last_blk);
        if (inc_size <= remain_size) // 当前块有足够空间 直接分配
        {
            last_blk->size += inc_size;
            // pktbuf->total_size += inc_size;
        }
        else
        {
            pktblk_t* new_blk = pktblock_alloc_list(inc_size - remain_size, false);
            if (new_blk == NULL)
            {
                dbug_error("pktblock alloc list failed");
                return NET_ERR_MEM;
            }
            last_blk->size += remain_size;
            // pktbuf->total_size += remain_size;
            pktbuf_insert_blk_list(pktbuf, new_blk, false);
        }
        pktbuf->total_size = new_size;
    }
    else // 缩小
    {
        int total_size = 0;
        pktblk_t* tail = NULL;
        for (tail = pktbuf_first_blk(pktbuf); tail != NULL; tail = pktblock_get_next(tail))
        {
            total_size += (int)tail->size;
            if (total_size >= new_size)
            {
                break;
            }
        }
        if (tail == NULL)
        {
            dbug_error("pktbuf resize failed,calc size less than new size");
            return NET_ERR_SYS;
        }
        total_size = 0;
        // 处理最后一个块
        pktblk_t* curr = pktblock_get_next(tail);
        while (curr)
        {
            pktblk_t* next = pktblock_get_next(curr);
            total_size += (int)curr->size;
            nlist_remove(&pktbuf->blk_list, &curr->node);
            pktblock_free(curr);
            curr = next;
        }

        tail->size -= pktbuf->total_size - total_size - new_size;
        pktbuf->total_size = new_size;
    }

    display_check_buf(pktbuf);
    return NET_ERR_OK;
}

net_err_t join_pktbuf(pktbuf_t* dst, pktbuf_t* src)
{
    if (!dst || !src)
    {
        return NET_ERR_INVALID_PARAM;
    }

    pktblk_t* first = NULL;
    while ((first = pktbuf_first_blk(src)))
    {
        nlist_remove_first(&src->blk_list);
        pktbuf_insert_blk_list(dst, first, false);
    }

    // 释放 src
    pktbuf_free(src);

    display_check_buf(dst);
    return NET_ERR_OK;
}

net_err_t pktbuf_set_cont(pktbuf_t* pktbuf, const int size)
{
    if (size > (int)pktbuf->total_size)
    {
        dbug_error("pktbuf set cont size too large,size=%d", size);
        return NET_ERR_INVALID_PARAM;
    }
    if (size > PKTBUF_PAYLOAD_SIZE)
    {
        dbug_error("pktbuf set cont size too large for block,size=%d", size);
        return NET_ERR_INVALID_PARAM;
    }

    pktblk_t* first_blk = pktbuf_first_blk(pktbuf);
    if (!first_blk)
    {
        dbug_error("pktbuf set cont failed,buf is empty");
        return NET_ERR_SYS;
    }

    // 是否已经是连续的
    if (first_blk->size >= (uint32_t)size)
    {
        display_check_buf(pktbuf);
        return NET_ERR_OK;
    }

    uint8_t* dist = first_blk->payload;
    for (int i = 0; i < first_blk->size; ++i)
    {
        *dist++ = first_blk->data[i];
    }
    first_blk->data = first_blk->payload;
    int remain_size = size - (int)first_blk->size;
    pktblk_t* curr_blk = pktblock_get_next(first_blk);
    while (remain_size && curr_blk)
    {
        const int curr_size = curr_blk->size <= (uint32_t)remain_size ? (int)curr_blk->size : remain_size;
        plat_memcpy(dist, curr_blk->data, curr_size);
        dist += curr_size;
        curr_blk->data += curr_size;
        curr_blk->size -= curr_size;
        first_blk->size += curr_size;
        remain_size -= curr_size;
        if (curr_blk->size == 0)
        {
            pktblk_t* next_blk = pktblock_get_next(curr_blk);
            nlist_remove(&pktbuf->blk_list, &curr_blk->node);
            pktblock_free(curr_blk);
            curr_blk = next_blk;
        }
    }
    display_check_buf(pktbuf);
    return NET_ERR_OK;
}
