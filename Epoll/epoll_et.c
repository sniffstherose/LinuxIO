#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

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
    int epfd = epoll_create(100);
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
                    int flag = fcntl(connfd, F_GETFL);
                    flag |= O_NONBLOCK;
                    fcntl(connfd, F_SETFL, flag);
                    event.data.fd = connfd;
                    event.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
                } else {    // 处理事件
                    if (events[i].events & EPOLLOUT) {
                        continue;
                    }
                    char recvbuf[5] = {0};
                    while (ret = read(curfd, recvbuf, 5) > 0) {
                        char test_buf[6] = {0};
                        strcpy(test_buf, recvbuf);
                        printf("recvbuf: %s\n", test_buf);
                        write(curfd, test_buf, strlen(test_buf));
                        memset(recvbuf, 0, sizeof(recvbuf));
                    }
                    if (ret == -1) {
                        perror("read");
                        exit(EXIT_FAILURE);
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