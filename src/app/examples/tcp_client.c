#include "common.h"
#include "net_api.h"

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

    if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        close(fd);
        return -1;
    }

    const char* msg = "Hello, TCP Server!";
    ssize_t sent_size = send(fd, msg, strlen(msg), 0);
    if (sent_size < 0)
    {
        perror("send failed");
        close(fd);
        return -1;
    }
    printf("Sent message to server: %s\n", msg);

    // 读取回复
    char buffer[1024];
    ssize_t recv_size = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (recv_size < 0)
    {
        perror("recv failed");
        close(fd);
        return -1;
    }
    buffer[recv_size] = '\0';
    printf("Received message from server: %s\n", buffer);
}
