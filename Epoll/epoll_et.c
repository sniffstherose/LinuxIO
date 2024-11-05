#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main()
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(6789);
    bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    listen(listen_fd, 8);

    int epfd = epoll_create(100);
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event);
    if (ret == -1) {
        perror("epoll_ctl");
        exit(-1);
    }

    struct epoll_event events[1024];
    while (1) {
        int num = epoll_wait(epfd, events, 1024, -1);
        if (num == -1) {
            perror("epoll_wait");
            exit(-1);
        }
        for (int i = 0; i < num; ++i) {
            int curfd = events[i].data.fd;
            if (curfd == listen_fd) {
                struct sockaddr_in client_addr;
                // memset(&client_addr, 0, sizeof(client_addr));
                socklen_t client_addr_len = sizeof(client_addr);
                int cfd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);

                // char client_ip[16];
                // char *temp = inet_ntoa(client_addr.sin_addr);
                // strcpy(client_ip, temp);
                // printf("IP为%s, 端口号为%d的客户端已连接\n", client_ip, ntohs(client_addr.sin_port));

                int flag = fcntl(cfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(cfd, F_SETFL, flag);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &event);
            }
            else {
                if (events[i].events & EPOLLOUT) {
                    continue;
                }

                char recvbuf[10] = {0};
                int len = 0;
                while ((len = read(curfd, recvbuf, sizeof(recvbuf))) > 0) {
                    write(STDOUT_FILENO, recvbuf, strlen(recvbuf));
                    memset(recvbuf, 0, sizeof(recvbuf));
                }
                char *done = "done";
                write(curfd, done, strlen(done));
                if (len == -1) {
                    if (errno == EAGAIN) {
                    }
                    else {
                        perror("read");
                        exit(-1);
                    }
                }
                else if (len == 0) {
                    printf("client close\n");
                }
            }
        }
    }

    close(listen_fd);
    close(epfd);
    return 0;
}
