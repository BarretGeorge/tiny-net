#ifndef TINY_NET_PKTBUF_H
#define TINY_NET_PKTBUF_H

#include <stddef.h>

#include "net_err.h"
#include "nlist.h"
#include "net_cfg.h"
#include <stdint.h>

typedef struct pktblk_t
{
    nlist_node_t node;
    uint8_t* data;
    uint8_t payload[PKTBUF_PAYLOAD_SIZE];
    uint32_t size;
} pktblk_t;

typedef struct pktbuf_t
{
    uint32_t total_size;
    nlist_t blk_list;
    nlist_node_t node;
} pktbuf_t;

net_err_t pktbuf_init();

pktbuf_t* pktbuf_alloc(int size);

void pktbuf_free(pktbuf_t* pktbuf);

static inline pktblk_t* pktblock_get_next(const pktblk_t* block)
{
    nlist_node_t* next = nlist_node_next(&block->node);
    return nlist_entry(next, pktblk_t, node);
}

static inline pktblk_t* pktblock_get_prev(const pktblk_t* block)
{
    nlist_node_t* prev = nlist_node_prev(&block->node);
    return nlist_entry(prev, pktblk_t, node);
}

#endif //TINY_NET_PKTBUF_H
