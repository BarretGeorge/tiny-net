#ifndef TINY_NET_NET_CFG_H
#define TINY_NET_NET_CFG_H

// 大小端模式
// 如果是大端模式，定义为0
// 如果是小端模式，定义为1
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define NET_ENDIAN_LITTLE 1
#elif __BYTE_ORDER__== __ORDER_BIG_ENDIAN__
#define NET_ENDIAN_LITTLE 0
#else
// 默认小端模式
#define NET_ENDIAN_LITTLE 1
#endif


// 消息队列大小
#define EXMSG_QUEUE_SIZE 10

// 数据包缓冲区有效负载大小
#define PKTBUF_PAYLOAD_SIZE 128

// 数据包缓冲区块数量
#define PKTBUF_BLK_COUNT 100

// 数据包缓冲区数量
#define PKTBUF_BUF_COUNT 100

// 网络接口硬件地址长度
#define NETIF_HWADDR_LEN 10

// 网络接口名称最大长度
#define NETIF_NAME_LEN 10

// 网络接口输入队列大小
#define NETIF_IN_QUEUE_SIZE 128

// 网络接口输出队列大小
#define NETIF_OUT_QUEUE_SIZE 128

// 网络接口设备数量
#define NETIF_DEV_CNT 10

// 定时器名称最大长度
#define TIMER_NAME_LEN 16

#endif //TINY_NET_NET_CFG_H
