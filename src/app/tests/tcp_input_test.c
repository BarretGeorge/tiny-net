#include "tcp_in.h"
#include "tool.h"

int main()
{
    pktbuf_init();

    tcp_init();

    sock_t* tcp = tcp_create(AF_INET, IPPROTO_TCP);

    pktbuf_t* buf = pktbuf_alloc(100);
    ipaddr_t src_ip, dest_ip;
    ipaddr_from_buf(&src_ip, (uint8_t[]){192, 168, 1, 100});
    ipaddr_from_buf(&dest_ip, (uint8_t[]){192, 168, 1, 1});

    ipaddr_copy(&tcp->remote_ip, &src_ip);
    tcp->local_port = 80;
    ipaddr_copy(&tcp->local_ip, &dest_ip);
    tcp->remote_port = 12345;


    tcp_header_t header = {
        .src_port = x_ntohs(12345),
        .dest_port = x_ntohs(80),
        .seq_num = 0,
        .ack_num = 0,
        .f_data_offset = 5, // 20 bytes
        .window_size = x_ntohs(65535),
        .checksum = 0,
    };

    pktbuf_write(buf, (uint8_t*)&header, sizeof(header));

    pktbuf_write(buf, (uint8_t*)"Hello", 5);

    pktbuf_reset_access(buf);

    tcp_input(buf, &src_ip, &dest_ip);
    return 0;
}
