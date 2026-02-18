#ifndef TINY_NET_TCP_STATE_H
#define TINY_NET_TCP_STATE_H

#include "tcp.h"

const char* tcp_state_name(tcp_state_t state);

void tcp_set_state(tcp_t* tcp, tcp_state_t state);

#endif //TINY_NET_TCP_STATE_H