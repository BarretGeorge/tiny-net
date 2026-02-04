#ifndef TINY_NET_SOCK_H
#define TINY_NET_SOCK_H

#include "exmsg.h"
#include "net_err.h"

typedef int x_socklen_t;

struct sock_t;

struct x_socketaddr;

struct sock_req_t;

#define SOCK_WAIT_READ (1<<0)
#define SOCK_WAIT_WRITE (1<<1)
#define SOCK_WAIT_CONN (1<<2)
#define SOCK_WAIT_ALL (SOCK_WAIT_READ | SOCK_WAIT_WRITE | SOCK_WAIT_CONN)

typedef struct sock_wait_t
{
    sys_sem_t sem;
    net_err_t err;
    int waiting;
} sock_wait_t;

net_err_t sock_wait_init(sock_wait_t* wait);

void sock_wait_destroy(sock_wait_t* wait);

void sock_wait_add(sock_wait_t* wait, int timeout, struct sock_req_t* req);

net_err_t sock_wait_enter(const sock_wait_t* wait, int timeout);

void sock_wait_leave(sock_wait_t* wait, net_err_t err);

void sock_wakeup(const struct sock_t* sock, int type, net_err_t err);

typedef struct sock_ops_t
{
    net_err_t (*close)(struct sock_t* sock);
    net_err_t (*setsockopt)(struct sock_t* sock, int level, int opt_name, const void* opt_val, int opt_len);
    net_err_t (*sendto)(struct sock_t* sock, const uint8_t* buf, size_t len, int flags,
                        const struct x_socketaddr* dest, x_socklen_t dest_len, ssize_t* sent_size);
    net_err_t (*recvfrom)(struct sock_t* sock, uint8_t* buf, size_t len, int flags,
                          const struct x_socketaddr* src, x_socklen_t* src_len, ssize_t* recv_size);
    net_err_t (*setopt)(struct sock_t* sock, int level, int opt_name, const void* opt_val, int opt_len);
    void (*destroy)(struct sock_t* sock);
} sock_ops_t;

typedef struct sock_t
{
    uint16_t local_port;
    uint16_t remote_port;
    ipaddr_t local_ip;
    ipaddr_t remote_ip;
    const sock_ops_t* ops;
    int family;
    int protocol;
    net_err_t err;
    int recv_timeout;
    int send_timeout;

    sock_wait_t* recv_wait;
    sock_wait_t* send_wait;
    sock_wait_t* conn_wait;
    nlist_node_t node;
} sock_t;

typedef struct x_socket_t
{
    enum
    {
        SOCK_STATE_FREE = 0,
        SOCK_STATE_USED,
    } state;

    sock_t* sock;
} x_socket_t;

typedef struct sock_create_t
{
    int family;
    int type;
    int protocol;
} sock_create_t;

typedef struct sock_data_t
{
    uint8_t* buf;
    size_t len;
    int flags;
    struct x_socketaddr* addr;
    x_socklen_t* addrlen;
    ssize_t transferred_len;
} sock_data_t;

typedef struct sock_opt_t
{
    int level;
    int opt_name;
    const void* opt_val;
    int opt_len;
} sock_opt_t;

typedef struct sock_req_t
{
    int fd;
    sock_wait_t* wait;
    int wait_timeout;

    union
    {
        sock_create_t create;
        sock_data_t data;
        sock_opt_t opt;
    };
} sock_req_t;

net_err_t socket_init(void);

net_err_t socket_create_req_in(const func_msg_t* msg);

net_err_t socket_sendto_req_in(const func_msg_t* msg);

net_err_t socket_recvfrom_req_in(const func_msg_t* msg);

net_err_t socket_setsockopt_req_in(const func_msg_t* msg);

net_err_t socket_close_req_in(const func_msg_t* msg);

net_err_t sock_init(sock_t* sock, int family, int protocol, const sock_ops_t* ops);

net_err_t sock_setopt(sock_t* sock, int level, int opt_name, const void* opt_val, int opt_len);

void sock_free(const sock_t* sock);

#endif //TINY_NET_SOCK_H
