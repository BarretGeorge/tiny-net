#ifndef TINY_NET_NETIF_H
#define TINY_NET_NETIF_H

#include "ipaddr.h"
#include "nlist.h"
#include "fixq.h"
#include "net_cfg.h"
#include "net_err.h"

typedef struct netif_hwaddr_t
{
    uint8_t addr[NETIF_HWADDR_LEN];
    uint8_t len;
} netif_hwaddr_t;

typedef enum netif_type_t
{
    NETIF_TYPE_NONE = 0,
    // 以太网接口
    NETIF_TYPE_ETHERNET,
    // 回环接口
    NETIF_TYPE_LOOPBACK,
    // 无线网接口
    NETIF_TYPE_WIFI,
} netif_type_t;

struct netif_t;

typedef struct netif_open_options_t
{
    // 打开接口函数指针
    net_err_t (*open)(struct netif_t* netif, void* data);

    // 关闭接口函数指针
    net_err_t (*close)(struct netif_t* netif);

    // 发送数据函数指针
    net_err_t (*linkoutput)(struct netif_t* netif);
} netif_open_options_t;

typedef struct netif_t
{
    // Interface name
    char name[NETIF_NAME_LEN];
    // 硬件地址
    netif_hwaddr_t hwaddr;
    // IP地址
    ipaddr_t ipaddr;
    // 子网掩码
    ipaddr_t netmask;
    // 默认网关
    ipaddr_t gateway;
    // 接口类型
    netif_type_t type;
    // 最大传输单元
    int mtu;
    // 链表节点
    nlist_node_t node;
    // 接收队列
    fixq_t in_q;
    // 接收队列私有数据指针
    void* in_q_buf[NETIF_IN_QUEUE_SIZE];
    // 发送队列
    fixq_t out_q;
    // 发送队列私有数据指针
    void* out_q_buf[NETIF_OUT_QUEUE_SIZE];
    // 打开选项
    netif_open_options_t* opts;
    // 打开选项私有数据指针
    void* opts_data;

    // 接口状态
    enum
    {
        // 接口关闭状态
        NETIF_STATE_CLOSED = 0,
        // 接口打开状态
        NETIF_STATE_OPENED,
        // 接口活动状态
        NETIF_STATE_ACTIVE,
    } state;
} netif_t;

net_err_t netif_init();

netif_t* netif_open(const char* dev_name, const netif_open_options_t* opts, void* opts_data);

#endif //TINY_NET_NETIF_H
