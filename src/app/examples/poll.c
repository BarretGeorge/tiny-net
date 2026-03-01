#include <stdio.h>
#include "net_api.h"
#include "poll.h"

#define PORT 8888
#define MAX_CLIENTS 1024
#define BUFFER_SIZE 1024

int main()
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1; // 当前有效fd数量

    // 创建 socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    // 端口复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    // 监听
    if (listen(listen_fd, 128) < 0)
    {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // 初始化 poll
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;

    while (1)
    {
        int ret = poll(fds, nfds, -1);
        if (ret < 0)
        {
            perror("poll");
            break;
        }

        // 遍历所有 fd
        for (int i = 0; i < nfds; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                // 新连接
                if (fds[i].fd == listen_fd)
                {
                    int conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                    if (conn_fd < 0)
                    {
                        perror("accept");
                        continue;
                    }

                    printf("New client: %s:%d\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));

                    // 加入 poll 列表
                    fds[nfds].fd = conn_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                }
                // 客户端数据
                else
                {
                    char buffer[BUFFER_SIZE];
                    int n = read(fds[i].fd, buffer, sizeof(buffer));

                    if (n <= 0)
                    {
                        printf("Client disconnected\n");
                        close(fds[i].fd);

                        // 删除该 fd（用最后一个覆盖）
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        i--; // 重新检查当前位置
                    }
                    else
                    {
                        write(fds[i].fd, buffer, n); // echo 回去
                    }
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}
