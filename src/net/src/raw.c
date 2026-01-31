#include "raw.h"

#include "dbug.h"
#include "mblock.h"

static raw_t raw_tbl[RAW_MAX_NR];

static mblock_t raw_mblock;

static nlist_t raw_list;

net_err_t raw_init()
{
    plat_memset(raw_tbl, 0, sizeof(raw_tbl));

    nlist_init(&raw_list);

    mblock_init(&raw_mblock, raw_tbl, sizeof(raw_t), RAW_MAX_NR, NLOCKER_TYPE_NONE);
    return NET_ERR_OK;
}


sock_t* raw_create(const int family, const int protocol)
{
    static const sock_ops_t raw_ops;
    raw_t* raw = mblock_alloc(&raw_mblock, -1);
    if (raw == NULL)
    {
        dbug_error("raw_create: no memory for raw");
        return NULL;
    }

    net_err_t err = sock_init(&raw->base, family, protocol, &raw_ops);
    if (err != NET_ERR_OK)
    {
        dbug_error("raw_create: sock_init failed");
        mblock_free(&raw_mblock, raw);
        return NULL;
    }

    nlist_insert_last(&raw_list, &raw->base.node);

    return &raw->base;
}
