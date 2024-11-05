#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define SERVERIP "127.0.0.1"
#define PORT "6789"

int main() {
    // 1. 创建socket套接字，用于监听
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
}