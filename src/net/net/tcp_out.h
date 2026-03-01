#ifndef TINY_NET_TCP_OUT_H
#define TINY_NET_TCP_OUT_H

#include "net_err.h"
#include "tcp.h"

net_err_t tcp_send_reset(const tcp_seg_t* seg);

net_err_t tcp_send_syn(tcp_t* tcp);

net_err_t tcp_ack_process(tcp_t* tcp, const tcp_seg_t* seg);

net_err_t tcp_send_ack(tcp_t* tcp, tcp_seg_t* seg);

net_err_t tcp_send_fin(tcp_t* tcp);

net_err_t tcp_send_keep_alive(const tcp_t* tcp);

net_err_t tcp_send_reset_with_tcp(const tcp_t* tcp);

net_err_t tcp_transmit(tcp_t* tcp);

int tcp_write_send_buf(tcp_t* tcp, const uint8_t* buf, size_t len);

#endif //TINY_NET_TCP_OUT_H
