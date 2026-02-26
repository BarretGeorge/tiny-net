#include "tcp_in.h"

int main()
{
    pktbuf_t* buf = pktbuf_alloc(100);
    ipaddr_t src_ip, dest_ip;
    ipaddr_from_buf(&src_ip, (uint8_t[]){192, 168, 1, 100});
    ipaddr_from_buf(&dest_ip, (uint8_t[]){192, 168, 1, 1});


    tcp_header_t header = {
        .src_port = 12345,
        .dest_port = 80,
        .seq_num = 0,
        .ack_num = 0,
        .f_data_offset = 5, // 20 bytes
        .window_size = 1024,
        .checksum = 0,
    };

    pktbuf_write(buf, (uint8_t*)&header, sizeof(header));

    pktbuf_write(buf, (uint8_t*)"Hello", 5);

    pktbuf_reset_access(buf);

    tcp_input(buf, &src_ip, &dest_ip);
    return 0;
}
