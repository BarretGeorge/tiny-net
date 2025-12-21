#include "nlist.h"

#include <stdlib.h>

nlist_node_t* nlist_insert_node(nlist_t* list, nlist_node_t* prev, nlist_node_t* next, void* data)
{
    if (list == NULL)
    {
        return NULL;
    }
    // 创建节点
    nlist_node_t* node = malloc(sizeof(nlist_node_t));
    if (node == NULL)
    {
        return NULL;
    }
    nlist_node_init(node, data, prev, next);

    if (prev != NULL)
    {
        prev->next = node;
    }
    else
    {
        list->head = node;
    }

    if (next != NULL)
    {
        next->prev = node;
    }
    else
    {
        list->tail = node;
    }

    list->size++;
    return node;
}

nlist_node_t* nlist_insert_index(nlist_t* list, const int index, void* data)
{
    if (list == NULL || index < 0 || index > list->size)
    {
        return NULL;
    }

    // 头部插入
    if (index == 0)
    {
        return nlist_insert_node(list, NULL, list->head, data);
    }

    // 尾部插入
    if (index == list->size)
    {
        return nlist_insert_node(list, list->tail, NULL, data);
    }

    int pos = 0;
    nlist_node_t* temp = NULL;

    if (index < list->size / 2)
    {
        // 从前往后找插入位置
        temp = list->head;
        while (pos < index)
        {
            temp = temp->next;
            pos++;
        }
    }
    else
    {
        // 从后往前找插入位置
        temp = list->tail;
        while (pos < list->size - index - 1)
        {
            temp = temp->prev;
            pos++;
        }
    }
    return nlist_insert_node(list, temp->prev, temp, data);
}

nlist_node_t* nlist_inset_head(nlist_t* list, void* data)
{
    return nlist_insert_index(list, 0, data);
}

nlist_node_t* nlist_inset_tail(nlist_t* list, void* data)
{
    return nlist_insert_index(list, list->size, data);
}

void nlist_for_each(const nlist_t* list, const nlist_for_each_cb_t cb)
{
    if (list == NULL || cb == NULL)
    {
        return;
    }
    const nlist_node_t* temp = list->head;
    while (temp != NULL)
    {
        cb(temp->data);
        temp = temp->next;
    }
}

bool nlist_remove_head(nlist_t* list)
{
    if (list == NULL)
    {
        return false;
    }
    return nlist_remove_node(list, list->head, true);
}

bool nlist_remove_tail(nlist_t* list)
{
    if (list == NULL)
    {
        return false;
    }
    return nlist_remove_node(list, list->tail, true);
}

bool nlist_remove_node(nlist_t* list, nlist_node_t* node, bool free_node)
{
    if (list == NULL || node == NULL)
    {
        return false;
    }

    if (node->prev == NULL) // 删除头节点
    {
        list->head = node->next;
    }
    else
    {
        node->prev->next = node->next;
    }

    if (node->next == NULL) // 删除尾节点
    {
        list->tail = node->prev;
    }
    else
    {
        node->next->prev = node->prev;
    }

    node->prev = node->next = NULL;
    if (free_node)
    {
        node->data = NULL;
        free(node);
    }

    list->size--;
    return true;
}

bool nlist_remove_index(nlist_t* list, const int index)
{
    if (list == NULL || index < 0 || index > list->size - 1)
    {
        return false;
    }
    if (index == 0)
    {
        return nlist_remove_node(list, list->head, true);
    }
    if (index == list->size - 1)
    {
        return nlist_remove_node(list, list->tail, true);
    }

    int pos = 0;
    nlist_node_t* temp = NULL;
    if (index < list->size / 2)
    {
        // 从前往后找删除节点
        temp = list->head;
        while (pos < index)
        {
            temp = temp->next;
            pos++;
        }
    }
    else
    {
        // 从后往前找删除节点
        temp = list->tail;
        while (pos < list->size - index - 1)
        {
            temp = temp->prev;
            pos++;
        }
    }
    return nlist_remove_node(list, temp, true);
}

void nlist_clear(nlist_t* list)
{
    if (list == NULL)
    {
        return;
    }
    nlist_node_t* temp = list->head;
    while (temp != NULL)
    {
        nlist_node_t* next = temp->next;
        temp->prev = temp->next = temp->data = NULL;
        free(temp);
        temp = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

nlist_node_t* nlist_remove_and_get_head(nlist_t* list)
{
    nlist_node_t* node = list->head;
    if (node != NULL)
    {
        nlist_remove_node(list, node, false);
    }
    return node;
}

nlist_node_t* nlist_remove_and_get_tail(nlist_t* list)
{
    nlist_node_t* node = list->tail;
    if (node != NULL)
    {
        nlist_remove_node(list, node, false);
    }
    return node;
}
