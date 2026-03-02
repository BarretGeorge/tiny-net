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
        printf("socket creation failed\n");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    server_addr.sin_addr.s_addr = inet_addr("110.242.74.102");

    if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("connect failed\n");
        close(fd);
        return -1;
    }

    // 设置keepalive选项
    int opt_val = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &opt_val, sizeof(opt_val));
    opt_val = 5; // 5秒空闲超时
    setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &opt_val, sizeof(opt_val));
    opt_val = 5; // 5秒探测间隔
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &opt_val, sizeof(opt_val));
    opt_val = 3; // 3次探测失败后断开
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &opt_val, sizeof(opt_val));

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
        printf("发送消息到服务器完成，发送内容长度%ld \n", send_size);
    }

    ssize_t recv_size = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (recv_size < 0)
    {
        printf("recv failed\n");
        close(fd);
        return -1;
    }
    buffer[recv_size] = '\0';
    printf("从服务器接收消息: %s\n", buffer);
    close(fd);
    return 0;
}
