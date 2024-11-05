#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVERIP "127.0.0.1"
#define PORT 6789

int main()
{
    //1. 创建连接套接字
    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd == -1) {
        perror("socket");
        exit(-1);
    }

    //2. 链接服务器
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVERIP);
    server_addr.sin_port = htons(PORT);
    int ret = connect(connfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("connect");
        exit(-1);
    }

    //通信
    int num = 0;
    while (1) {
        char sendbuf[1024] = {0};
        // sprintf(sendbuf, "send data %d\n", num++);
        fgets(sendbuf, sizeof(sendbuf), stdin);
        write(connfd, sendbuf, strlen(sendbuf) + 1);
        memset(sendbuf, 0, sizeof(sendbuf));
        ret = read(connfd, sendbuf, sizeof(sendbuf));
        if (ret > 0) {
            printf("接收到数据: %s\n", sendbuf);
        }
        else if (ret == 0) {
            printf("服务器断开连接\n");
            break;
        }
        else {
            perror("read");
            exit(-1);
        }
        sleep(1);
    }

    close(connfd);
    return 0;
}