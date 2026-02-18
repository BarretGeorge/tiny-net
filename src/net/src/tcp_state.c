#include "tcp_state.h"
#include "dbug.h"

const char* tcp_state_name(const tcp_state_t state)
{
    static const char* state_names[TCP_STATE_MAX] = {
        "CLOSE",
        "LISTEN",
        "SYN_SENT",
        "SYN_RECEIVED",
        "ESTABLISHED",
        "FIN_WAIT_1",
        "FIN_WAIT_2",
        "CLOSE_WAIT",
        "CLOSING",
        "LAST_ACK",
        "TIME_WAIT"
    };
    return state < TCP_STATE_MAX ? state_names[state] : "UNKNOWN";
}

void tcp_set_state(tcp_t* tcp, const tcp_state_t state)
{
    dbug_info(DBG_MOD_TCP, "TCP state change: %s -> %s",
              tcp_state_name(tcp->state), tcp_state_name(state));
    tcp->state = state;
}
