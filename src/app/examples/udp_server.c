#include "common.h"
#include "dbug_module.h"
#include "net_api.h"
#include <stdio.h>
#include <string.h>

int main()
{
    tiny_net_init();
    dbug_module_enable_only(DBG_MOD_UDP);

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9999);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct socketaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(fd);
        return -1;
    }

    printf("UDP server listening on port 9999...\n");

    char buffer[1024];
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        socklen_t addr_len = sizeof(client_addr);
        ssize_t recv_bytes = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
                                      (struct socketaddr*)&client_addr, &addr_len);
        if (recv_bytes < 0)
        {
            perror("recvfrom failed");
            break;
        }
        buffer[recv_bytes] = '\0';
        printf("Received %zd bytes from %s:%d: %s\n", recv_bytes,
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
    }

    close(fd);
    return 0;
}
