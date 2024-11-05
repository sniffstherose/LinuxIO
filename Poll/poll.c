#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVERIP "127.0.0.1"
#define PORT 6789

int main() {
    // 1. 创建socket套接字，用于监听
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. 绑定地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(PORT);
    int ret = bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 3. 监听
    ret = listen(listenfd, 8);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[1024];
    for (int i = 0; i < 1024; ++i) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    fds[0].fd = listenfd;
    int nfds = 0;
    while (1) {
        // 使用poll，永久阻塞
        int num = poll(fds, nfds + 1, -1);
        if (num == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        } else if (num == 0) {
            continue;
        } else {
            // 判断是否有客户端连接
            if (fds[0].revents & POLLIN) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
                if (connfd == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                char client_ip[16] = {0};
                inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
                unsigned short client_port = ntohs(client_addr.sin_port);
                printf("ip: %s, port: %d\n", client_ip, client_port);
                for (int i = 1; i < 1024; ++i) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = connfd;
                        fds[i].events = POLLIN;
                        break;
                    }
                }
                nfds = nfds > connfd ? nfds : connfd;
            }
            char recvbuf[1024] = {0};
            for (int i = 1; i <= nfds; ++i) {
                if (fds[i].fd != -1 && fds[i].revents & POLLIN) {
                    ret = read(fds[i].fd, recvbuf, sizeof(recvbuf));
                    if (ret == -1) {
                        perror("read");
                        exit(EXIT_FAILURE);
                    } else if (ret > 0) {
                        printf("recv client data: %s\n", recvbuf);
                        write(fds[i].fd, recvbuf, strlen(recvbuf));
                    } else {
                        printf("client closed...\n");
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        break;
                    }
                }
            }
        }
    }
    close(listenfd);
    return 0;
}