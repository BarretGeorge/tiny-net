#ifndef TINY_NET_NLIST_H
#define TINY_NET_NLIST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct nlist_node_t
{
    void* data;
    struct nlist_node_t* prev;
    struct nlist_node_t* next;
} nlist_node_t;

typedef void (*nlist_for_each_cb_t)(void* data);

static inline void nlist_node_init(nlist_node_t* node, void* data, nlist_node_t* prev, nlist_node_t* next)
{
    node->data = data;
    node->prev = prev;
    node->next = next;
}

typedef struct nlist_t
{
    nlist_node_t* head;
    nlist_node_t* tail;
    int size;
} nlist_t;

static inline void nlist_init(nlist_t* list)
{
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

static inline bool nlist_is_empty(const nlist_t* list)
{
    return list->size == 0;
}

static inline int nlist_size(const nlist_t* list)
{
    return list->size;
}

void nlist_insert_before(nlist_t* list, nlist_node_t* next, nlist_node_t* node);

void nlist_insert_after(nlist_t* list, nlist_node_t* prev, nlist_node_t* node);

nlist_node_t* nlist_inset_tail(nlist_t* list, nlist_node_t* node);

nlist_node_t* nlist_inset_head(nlist_t* list, nlist_node_t* node);

nlist_node_t* nlist_insert(nlist_t* list, nlist_node_t* prev, nlist_node_t* next, nlist_node_t* node);

nlist_node_t* nlist_insert_index(nlist_t* list, int index, nlist_node_t* node);

void nlist_for_each(const nlist_t* list, nlist_for_each_cb_t cb);

bool nlist_remove_head(nlist_t* list);

bool nlist_remove_tail(nlist_t* list);

bool nlist_remove_node(nlist_t* list, nlist_node_t* node);

bool nlist_remove_index(nlist_t* list, int index);

void nlist_clear(nlist_t* list);

nlist_node_t* nlist_remove_and_get_head(nlist_t* list);

nlist_node_t* nlist_remove_and_get_tail(nlist_t* list);

#endif //TINY_NET_NLIST_H
