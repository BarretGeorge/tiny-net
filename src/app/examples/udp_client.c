#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main()
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
    {
        perror("socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9999);
    server_addr.sin_addr.s_addr = inet_addr("192.168.100.120");
    // if (connect(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    // {
    //     perror("connect failed");
    //     close(fd);
    //     return -1;
    // }

    char msg[] = "Hello, UDP Server!";
    for (int i = 0; i < 10; i++)
    {
        ssize_t sent_bytes = sendto(fd, msg, sizeof(msg), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent_bytes < 0)
        {
            perror("send failed");
            close(fd);
            return -1;
        }
        printf("Sent %zd bytes: %s\n", sent_bytes, msg);
        sleep(1);
    }
    return 0;
}
