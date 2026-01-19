#ifndef TINY_NET_NET_ERR_H
#define TINY_NET_NET_ERR_H

typedef enum net_err_t
{
    NET_ERR_OK = 0,
    NET_ERR_SYS = -1, // 系统错误
    NET_ERR_MEM = -2, // 内存错误
    NET_ERR_INVALID_PARAM = -3, // 无效参数
    NET_ERR_INVALID_STATE = -4, // 无效状态
    NET_ERR_IO = -5, // 输入输出错误
    NET_ERR_FRAME = -6, // 数据包格式错误
    NET_ERR_TIMEOUT = -7, // 超时
    NET_ERR_TARGET_ADDR_MATCH = -8, // 目标地址不匹配
} net_err_t;

#endif //TINY_NET_NET_ERR_H
