#ifndef TINY_NET_TCP_OUT_H
#define TINY_NET_TCP_OUT_H

#include "net_err.h"
#include "tcp.h"

net_err_t tcp_send_reset(const tcp_seg_t* seg);

#endif //TINY_NET_TCP_OUT_H