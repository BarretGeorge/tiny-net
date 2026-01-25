#ifndef TINY_NET_NETIF_H
#define TINY_NET_NETIF_H

#include "ipaddr.h"
#include "nlist.h"
#include "fixq.h"
#include "net_cfg.h"
#include "net_err.h"
#include "pktbuf.h"

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
    // 接口类型数量
    NETIF_TYPE_SIZE,
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

struct link_layer_t;

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
    // 链路层指针
    const struct link_layer_t* link_layer;

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

typedef struct link_layer_t
{
    // 链路层类型
    netif_type_t type;
    // 打开函数指针
    net_err_t (*open)(netif_t* netif);
    // 关闭函数指针
    void (*close)(netif_t* netif);
    // 输入函数指针
    net_err_t (*input)(netif_t* netif, pktbuf_t* buf);
    // 输出函数指针
    net_err_t (*output)(netif_t* netif, ipaddr_t* ipaddr, pktbuf_t* buf);
} link_layer_t;

// 初始化网卡子系统
net_err_t netif_init();

// 打开网卡接口
netif_t* netif_open(const char* dev_name, const netif_open_options_t* opts, void* opts_data);

// 设置网卡硬件地址
net_err_t netif_set_hwaddr(netif_t* netif, const uint8_t* hwaddr, uint8_t hwaddr_len);

// 设置网卡IP地址、子网掩码和默认网关
net_err_t netif_set_addr(netif_t* netif, const ipaddr_t* ipaddr, const ipaddr_t* netmask, const ipaddr_t* gateway);

// 设置网卡为活动状态
net_err_t netif_set_active(netif_t* netif);

// 设置网卡为非活动状态
net_err_t netif_set_inactive(netif_t* netif);

// 关闭网卡接口
net_err_t netif_close(netif_t* netif);

// 设置默认网卡
net_err_t netif_set_default(netif_t* netif);

// 将数据包放入网卡的接收队列
net_err_t netif_put_in(netif_t* netif, pktbuf_t* buf, int tmo);

// 从网卡的接收队列获取数据包
pktbuf_t* netif_get_in(netif_t* netif, int tmo);

// 将数据包放入网卡的发送队列
net_err_t netif_put_out(netif_t* netif, pktbuf_t* buf, int tmo);

// 从网卡的发送队列获取数据包
pktbuf_t* netif_get_out(netif_t* netif, int tmo);

// 通过网卡发送数据包
net_err_t netif_out(netif_t* netif, ipaddr_t* ipaddr, pktbuf_t* buf);

// 注册链路层
net_err_t netif_register_link_layer(const link_layer_t* ll);

// 获取默认网卡
netif_t* netif_get_default();

#endif //TINY_NET_NETIF_H
