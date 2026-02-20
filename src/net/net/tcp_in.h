#ifndef TINY_NET_TCP_IN_H
#define TINY_NET_TCP_IN_H

#include "ipaddr.h"
#include "net_err.h"
#include "pktbuf.h"
#include "tcp.h"

net_err_t tcp_input(pktbuf_t* buf, const ipaddr_t* src_ip, const ipaddr_t* dest_ip);

net_err_t tcp_data_in(tcp_t* tcp, tcp_seg_t* seg);

#endif //TINY_NET_TCP_IN_H
