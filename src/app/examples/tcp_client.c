#include "common.h"
#include "net_api.h"
#include "dbug_module.h"

int main()
{
    dbug_module_enable_only(DBG_MOD_TCP);
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
    server_addr.sin_addr.s_addr = inet_addr("192.168.56.4");

    if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        close(fd);
        return -1;
    }

    char buffer[4096];
    for (int i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = (char)('A' + i % 26);
    }

    for (int i = 0; i < 3; i++)
    {
        ssize_t send_size = send(fd, buffer, sizeof(buffer) - 1, 0);
        if (send_size < 0)
        {
            perror("send failed");
            close(fd);
            return -1;
        }
        printf("发送消息到服务器: %s\n", buffer);
    }

    close(fd);
    return 0;


    ssize_t recv_size = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (recv_size < 0)
    {
        perror("recv failed");
        close(fd);
        return -1;
    }
    buffer[recv_size] = '\0';
    printf("从服务器接收消息: %s\n", buffer);
    close(fd);
    return 0;
}
