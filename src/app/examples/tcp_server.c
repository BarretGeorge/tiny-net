#include "net_api.h"
#include "common.h"

int main()
{
    tiny_net_init();

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9999);
    server_addr.sin_addr.s_addr = inet_addr("192.168.3.95");

    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(fd);
        return -1;
    }

    if (listen(fd, 5) < 0)
    {
        perror("listen failed");
        close(fd);
        return -1;
    }

    sleep(10000);
    return 0;
}
