#ifndef TINY_NET_NET_CFG_H
#define TINY_NET_NET_CFG_H

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

#endif //TINY_NET_NET_CFG_H
