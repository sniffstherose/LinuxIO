#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

#define SERVERIP "127.0.0.1"
#define PORT 6789

int main() {
    // 1. 创建监听套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. 绑定
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
    // 创建epoll实例
    int epfd = epoll_create(1024);
    // 监听文件描述符加入实例
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listenfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);
    if (ret == -1) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
    // 用来保存返回的信息
    struct epoll_event events[1024];
    while (1) {
        // 使用epoll_wait，并设置永久阻塞
        int nums = epoll_wait(epfd, events, 1024, -1);
        if (nums == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        } else if (nums == 0) {
            continue;
        } else {
            for (int i = 0; i < nums; ++i) {
                int curfd = events[i].data.fd;
                // 有新的连接
                if (curfd == listenfd) {
                    struct sockaddr_in client_addr;
                    socklen_t len = sizeof(client_addr);
                    int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &len);
                    if (connfd == -1) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    printf("new connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    event.data.fd = connfd;
                    event.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
                } else {    // 处理事件
                    if (events[i].events & EPOLLOUT) {
                        continue;
                    }
                    char recvbuf[1024] = {0};
                    ret = read(curfd, recvbuf, sizeof(recvbuf));
                    if (ret == -1) {
                        perror("read");
                        exit(EXIT_FAILURE);
                    } else if (ret > 0) {
                        printf("recvbuf: %s\n", recvbuf);
                        write(curfd, recvbuf, strlen(recvbuf));
                    } else {    // 断开连接
                        printf("client closed...\n");
                        close(curfd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);
                        break;
                    }
                }
            }
        }

    }
    close(listenfd);
    close(epfd);
    return 0;
}